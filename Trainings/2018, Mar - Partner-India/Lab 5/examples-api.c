#include <stddef.h>
#include <shell/tash.h>

extern int security_test_main(int argc, char *argv[]);
extern int artik_onboarding_main(int argc, char *argv[]);
extern int mqtt_main(int argc, char *argv[]);

static tash_cmdlist_t atk_cmds[] = {
    {"onboard", artik_onboarding_main, TASH_EXECMD_SYNC},
    {"security", security_test_main, TASH_EXECMD_SYNC},
	//{"mqtt", mqtt_main, TASH_EXECMD_SYNC},
    {NULL, NULL, 0}
};

int main(int argc, char *argv[])
{
#ifdef CONFIG_TASH
    /* add tash command */
    tash_cmdlist_install(atk_cmds);
#endif

    return 0;
}

