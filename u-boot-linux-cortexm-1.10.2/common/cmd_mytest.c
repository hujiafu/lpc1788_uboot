
#include <common.h>
#include <command.h>

int do_mytest(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]){

	unsigned int *clk_div;
	long long div;
	char *endp;
	
	clk_div = (unsigned int *)0x400FC1B8;

	div = simple_strtoul(argv[1], &endp, 10);


	printf("argc = %d, argv[1] = 0x%x\n", argc, div);

	if(argc != 2){
		printf("only support 2 argc\n");
		return -1;
	}

	*clk_div = div;
	printf("REG LCD_CFG = 0x%x\n",*clk_div);
	return 1;
}

U_BOOT_CMD
(
	mycmd, 2, 1, do_mytest,
	"mycmd command usage",
	"mycmd help"
);

