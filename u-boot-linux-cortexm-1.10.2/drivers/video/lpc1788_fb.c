#include <common.h>
#include <video_fb.h>
#include "videomodes.h"
GraphicDevice smi;

#define VIDEO_MEM_SIZE	0x200000

extern void board_video_init(GraphicDevice *pGD);

void *video_hw_init(void){

	GraphicDevice *pGD = (GraphicDevice *)&smi;
	int videomode;
	unsigned long hsynch, vsynch;
	char *penv;
	int tmp, i, bits_per_pixel;
	struct ctfb_res_modes *res_mode;
	struct ctfb_res_modes var_mode;

	printf("Video:");
	
	tmp = 0;

	memset(&smi, 0, sizeof(GraphicDevice));

	videomode = CONFIG_SYS_DEFAULT_VIDEO_MODE;

	if((penv = getenv("videomode")) != NULL){
		if(penv[0] <= '9'){
			videomode = (int)simple_strtoul(penv, NULL, 16);
			tmp = 1;
		}
	}else{
		tmp = 1;
	}

	if(tmp){
		for(i=0; i<VESA_MODES_COUNT; i++){
			if(vesa_modes[i].vesanr == videomode)
			  break;
		}
		if(i == VESA_MODES_COUNT){
			printf("no VESA mode found, switch to mode 0x%x", CONFIG_SYS_DEFAULT_VIDEO_MODE);
			i = 0;
		}
		res_mode = (struct ctfb_res_modes *)&res_mode_init[vesa_modes[i].resindex];
		bits_per_pixel = vesa_modes[i].bits_per_pixel;
	}else{
		res_mode = (struct ctfb_res_modes *)&var_mode;
		bits_per_pixel = video_get_params(res_mode, penv);
	}
	
	//sprintf(pGD->modeIdent, "%dx%dx%d %ldkHz %ldHz", res_mode->xres, res_mode->yres, bits_per_pixel, (hsynch/1000), (vsynch/1000));
	pGD->modeIdent[0] = res_mode->pixclock;
	pGD->modeIdent[1] = res_mode->left_margin;
	pGD->modeIdent[2] = res_mode->right_margin;
	pGD->modeIdent[3] = res_mode->upper_margin;
	pGD->modeIdent[4] = res_mode->lower_margin;
	pGD->modeIdent[5] = res_mode->hsync_len;
	pGD->modeIdent[6] = res_mode->vsync_len;

	pGD->winSizeX = res_mode->xres;
	pGD->winSizeY = res_mode->yres;
	pGD->plnSizeX = res_mode->xres;
	pGD->plnSizeY = res_mode->yres;

	switch(bits_per_pixel){
		case 8:
			pGD->gdfBytesPP = 1;
			pGD->gdfIndex = GDF__8BIT_INDEX;
			break;
		case 15:
			pGD->gdfBytesPP = 2;
			pGD->gdfIndex = GDF_15BIT_555RGB;
			break;
		case 16:
			pGD->gdfBytesPP = 2;
			pGD->gdfIndex = GDF_16BIT_565RGB;
			break;
		case 24:
			pGD->gdfBytesPP = 3;
			pGD->gdfIndex = GDF_24BIT_888RGB;
			break;
	}
		
	pGD->frameAdrs = LCD_VIDEO_ADDR;
	pGD->memSize = (pGD->winSizeX) * (pGD->winSizeY) * (pGD->gdfBytesPP);
	pGD->isaBase = 0;
	pGD->pciBase = 0;
	/* Cursor Start Address */
	pGD->dprBase = 0;
	pGD->vprBase = 0;
	pGD->cprBase = 0;

	board_video_init(pGD);

	/* Clear video memory */
	memset((void *)pGD->frameAdrs, 0, pGD->memSize);

	return ((void *)&smi);

}

void video_set_lut(unsigned int index, unsigned char r, unsigned char g, unsigned char b){

}

