#include <kernel.h>

int main()
{
	int i;

	bios_putstr("\n\n\n\n\n\n\n\n\n\n\n\n");
	bios_putstr("                                            原 神，启动！\n");
	bios_putstr("                                             O S, boot!");
	bios_putstr("\n\n\n\n\n\n\n\n\n\n\n\n");
	bios_putstr("                      抵制不良游戏，拒绝盗版游戏。 注意自我保护，谨防受骗上当。\n");
	bios_putstr("                      适度游戏益脑，沉迷游戏伤身。 合理安排时间，享受健康生活。\n");
	while ((i = bios_getchar()) == -1)
		;
	if (i == 'k')
		return 1;
	return 0;
}