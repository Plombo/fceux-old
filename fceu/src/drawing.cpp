#include "types.h"
#include "fceu.h"
#include "drawing.h"
#include "video.h"

static uint8 Font6x5[594] =
{
	4,  0,  0,  0,  0,  0,
	3, 64, 64, 64,  0, 64,
	5, 80, 80,  0,  0,  0,
	6, 80,248, 80,248, 80,
	6,112,160,112, 40,240,
	6,136, 16, 32, 64,136,
	6, 96, 96,104,144,104,
	3, 64, 64,  0,  0,  0,
	4, 32, 64, 64, 64, 32,
	4, 64, 32, 32, 32, 64,
	7, 72, 48,252, 48, 72,
	6, 32, 32,248, 32, 32,
	3,  0,  0,  0, 64,128,
	5,  0,  0,240,  0,  0,
	3,  0,  0,  0,  0, 64,
	6,  8, 16, 32, 64,128,
	5,240,144,144,144,240, //0
	5, 64,192, 64, 64,224, //1
	5, 96,144, 32, 64,240, //2
	5,240, 16, 96, 16,224, //3
	5, 80,144,240, 16, 16, //4
	5,240,128,224, 16,224, //5
	5, 96,128,224,144, 96, //6
	5,240, 16, 32, 32, 64, //7
	5, 96,144, 96,144, 96, //8
	5, 96,144,112, 16, 96, //9
	3,  0, 64,  0, 64,  0,
	3,  0, 64,  0, 64,128,
	4, 32, 64,128, 64, 32,
	4,  0,224,  0,224,  0,
	4,128, 64, 32, 64,128,
	5, 96,144, 32,  0, 32,
	5, 96,144,176,128, 96,
	5, 96,144,240,144,144,
	5,224,144,224,144,224,
	5,112,128,128,128,112,
	5,224,144,144,144,224,
	5,240,128,224,128,240,
	5,240,128,224,128,128,
	5,112,128,176,144,112,
	5,144,144,240,144,144,
	4,224, 64, 64, 64,224,
	5, 16, 16, 16,144, 96,
	5,144,160,192,160,144,
	5,128,128,128,128,240,
	6,136,216,168,136,136,
	6,136,200,168,152,136,
	5, 96,144,144,144, 96,
	5,224,144,224,128,128,
	5, 96,144,144,176,112,
	5,224,144,224,160,144,
	5, 96,128, 96, 16,224,
	6,248, 32, 32, 32, 32,
	5,144,144,144,144, 96,
	6,136,136, 80, 80, 32,
	6,136,136,136,168, 80,
	6,136, 80, 32, 80,136,
	6,136, 80, 32, 32, 32,
	6,248, 16, 32, 64,248,
	3,192,128,128,128,192,
	6,128, 64, 32, 16,  8,
	3,192, 64, 64, 64,192,
	4, 64,160,  0,  0,  0,
	5,  0,  0,  0,  0,240,
	3,128, 64,  0,  0,  0,
	5, 96, 16,112,144,112,
	5,128,128,224,144,224,
	4,  0, 96,128,128, 96,
	5, 16, 16,112,144,112,
	5,  0, 96,240,128, 96,
	4, 96,128,192,128,128,
	4,  0,224,224, 32,192,
	5,128,128,224,144,144,
	4, 64,  0,192, 64,224,
	3, 64,  0, 64, 64,192,
	5,128,160,192,160,144,
	4,192, 64, 64, 64,224,
	6,  0,208,168,168,136,
	5,  0,224,144,144,144,
	5,  0, 96,144,144, 96,
	5,  0,224,144,224,128,
	5,  0,112,144,112, 16,
	4,  0, 96,128,128,128,
	5,  0, 96,224, 16,224,
	4, 64,224, 64, 64, 32,
	5,  0,144,144,144,112,
	6,  0,136,136, 80, 32,
	6,  0,136,136,168, 80,
	5,  0,144, 96, 96,144,
	5,  0,144,112, 16, 96,
	5,  0,240, 32, 64,240,
	4, 96, 64,128, 64, 96,
	3, 64, 64, 64, 64, 64,
	4,192, 64, 32, 64,192,
	5, 80,160,  0,  0,  0
};

void DrawTextLineBG(uint8 *dest)
{
	int x,y;
	static int otable[7]={81,49,30,17,8,3,0};
	//100,40,15,10,7,5,2};
	for(y=0;y<14;y++)
	{
		int offs;

		if(y>=7) offs=otable[13-y];
		else offs=otable[y];  

		for(x=offs;x<(256-offs);x++)
		{
			// Choose the dimmest set of colours and then dim that
			dest[y*256+x]=(dest[y*256+x]&0x0F)|0xC0;
		}
	}
}


void DrawMessage(bool beforeMovie)
{
	if(guiMessage.howlong)
	{
		//don't display movie messages if we're not before the movie
		if(beforeMovie && !guiMessage.isMovieMessage)
			return;

		uint8 *t;
		guiMessage.howlong--;
		t=XBuf+FCEU_TextScanlineOffsetFromBottom(16);

		/*
		FCEU palette:
		$00: [8] unvpalette found in palettes/palettes.h
		black, white, black, greyish, redish, bright green, bluish
		$80:
		nes palette
		$C0:
		dim version of nes palette

		*/

		if(t>=XBuf)
		{
			int color = 0x20;
			if(guiMessage.howlong == 39) color = 0x38;
			if(guiMessage.howlong <= 30) color = 0x2C;
			if(guiMessage.howlong <= 20) color = 0x1C;
			if(guiMessage.howlong <= 10) color = 0x11;
			if(guiMessage.howlong <= 5) color = 0x1;
			DrawTextTrans(t, 256, (uint8 *)guiMessage.errmsg, color+0x80);
		}
	}
}




static uint8 sstat[2541] =
{
	0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,
	0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x83,0x83,0x83,
	0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x80,0x83,
	0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,
	0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x80,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x81,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x80,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x81,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x81,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x81,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x80,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x80,0x80,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x83,0x83,
	0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x81,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,
	0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x80,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x80,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,
	0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x80,0x80,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,
	0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x83,0x80,0x80,0x81,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x80,0x83,0x83,0x81,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x81,0x81,0x81,0x83,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80
};





static uint8 play_slines[]=
{
	0, 0, 1,
	1, 0, 2,
	2, 0, 3,
	3, 0, 4,
	4, 0, 5,
	5, 0, 6,
	6, 0, 7,
	7, 0, 8,
	8, 0, 7,
	9, 0, 6,
	10, 0, 5,
	11, 0, 4,
	12, 0, 3,
	13, 0, 2,
	14, 0, 1,
	99,
};

static uint8 record_slines[]=
{
	0, 5, 9,
	1, 3, 11,
	2, 2, 12,
	3, 1, 13,
	4, 1, 13,
	5, 0, 14,
	6, 0, 14,
	7, 0, 14,
	8, 0, 14,
	9, 0, 14,
	10, 1, 13,
	11, 1, 13,
	12, 2, 12,
	13, 3, 11,
	14, 5, 9,
	99,
};

static uint8 pause_slines[]=
{
	0, 2, 6,
	1, 2, 6,
	2, 2, 6,
	3, 2, 6,
	4, 2, 6,
	5, 2, 6,
	6, 2, 6,
	7, 2, 6,
	8, 2, 6,
	9, 2, 6,
	10, 2, 6,
	11, 2, 6,
	12, 2, 6,
	13, 2, 6,
	14, 2, 6,

	0, 9, 13,
	1, 9, 13,
	2, 9, 13,
	3, 9, 13,
	4, 9, 13,
	5, 9, 13,
	6, 9, 13,
	7, 9, 13,
	8, 9, 13,
	9, 9, 13,
	10, 9, 13,
	11, 9, 13,
	12, 9, 13,
	13, 9, 13,
	14, 9, 13,
	99,
};

static uint8 no_slines[]=
{
	99
};

static uint8* sline_icons[4]=
{
	no_slines,
	play_slines,
	record_slines,
	pause_slines
};

static void drawstatus(uint8* XBuf, int n, int y, int xofs)
{
	uint8* slines=sline_icons[n];
	int i;

	
	XBuf += FCEU_TextScanlineOffsetFromBottom(y) + 240 + 255 + xofs;
	for(i=0; slines[i]!=99; i+=3)
	{
		int y=slines[i];
		uint8* dest=XBuf+(y*256);
		int x;
		for(x=slines[i+1]; x!=slines[i+2]; ++x)
			dest[x]=0;
	}

	XBuf -= 255;
	for(i=0; slines[i]!=99; i+=3)
	{
		int y=slines[i];
		uint8* dest=XBuf+(y*256);
		int x;
		for(x=slines[i+1]; x!=slines[i+2]; ++x)
			dest[x]=4;
	}
}

/// this draws the recording icon (play/pause/record)
void FCEU_DrawRecordingStatus(uint8* XBuf)
{
	if(FCEUD_ShowStatusIcon())
	{
		bool hasPlayRecIcon = false;	
		if(FCEUI_IsMovieActive()>0)
		{
			drawstatus(XBuf,2,28,0);
			hasPlayRecIcon = true;
		}
		else if(FCEUI_IsMovieActive()<0)
		{
			drawstatus(XBuf,1,28,0);
			hasPlayRecIcon = true;
		}

		if(FCEUI_EmulationPaused())
			drawstatus(XBuf,3,28,hasPlayRecIcon?-16:0);
	}
}


void FCEU_DrawNumberRow(uint8 *XBuf, int *nstatus, int cur)
{
	uint8 *XBaf;
	int z,x,y;

	XBaf=XBuf - 4 + (FSettings.LastSLine-34)*256;
	if(XBaf>=XBuf)
		for(z=1;z<11;z++)
		{
			if(nstatus[z%10])
			{
				for(y=0;y<13;y++)
					for(x=0;x<21;x++)
						XBaf[y*256+x+z*21+z]=sstat[y*21+x+(z-1)*21*12]^0x80;
			} else {
				for(y=0;y<13;y++)
					for(x=0;x<21;x++)
						if(sstat[y*21+x+(z-1)*21*12]!=0x83)
							XBaf[y*256+x+z*21+z]=sstat[y*21+x+(z-1)*21*12]^0x80;

						else
							XBaf[y*256+x+z*21+z]=(XBaf[y*256+x+z*21+z]&0xF)|0xC0;
			}
			if(cur==z%10)
			{
				for(x=0;x<21;x++)
					XBaf[x+z*21+z*1]=4;
				for(x=1;x<12;x++)
				{
					XBaf[256*x+z*21+z*1]=
						XBaf[256*x+z*21+z*1+20]=4;
				}
				for(x=0;x<21;x++)
					XBaf[12*256+x+z*21+z*1]=4;
			}
		}
}  

static int FixJoedChar(uint8 ch)
{
	int c = ch; c -= 32;
	return (c < 0 || c > 98) ? 0 : c;
}
static int JoedCharWidth(uint8 ch)
{
	return Font6x5[FixJoedChar(ch)*6];
}

void DrawTextTrans(uint8 *dest, uint32 width, uint8 *textmsg, uint8 fgcolor)
{
	unsigned beginx=5, x=beginx;
	unsigned y=2;

	char target[64][256] = {{0}};

	assert(width==256);

	for(; *textmsg; ++textmsg)
	{
		int ch, wid;

		if(*textmsg == '\n') { x=beginx; y+=6; continue; }
		ch  = FixJoedChar(*textmsg);
		wid = JoedCharWidth(*textmsg);

		for(int ny=0; ny<5; ++ny)
		{
			uint8 d = Font6x5[ch*6 + 1+ny];
			for(int nx=0; nx<wid; ++nx)
			{
				int c = (d >> (7-nx)) & 1;
				if(c)
				{
					if(y+ny >= 16) goto textoverflow;
					target[y+ny][x+nx] = 2;
				}
				else
					target[y+ny][x+nx] = 1;
			}
		}
		x += wid;
		if(x >= width) { x=beginx; y+=6; }
	}
textoverflow:
	for(y=0; y<16; ++y)
		for(x=0; x<width; ++x)
		{
			int offs = y*width+x;
			int c = 0;

			c += target[y][x] * 600;

			x>=(     1) && (c += target[y][x-1] * 8);
			x<(width-1) && (c += target[y][x+1] * 8);
			y>=(     1) && (c += target[y-1][x] * 8);
			y<(16   -1) && (c += target[y+1][x] * 8);

			x>=(     1) && y>=(  1) && (c += target[y-1][x-1]*3);
			x<(width-1) && y>=(  1) && (c += target[y-1][x+1]*3);
			x>=(     1) && y<(16-1) && (c += target[y+1][x-1]*3);
			x<(width-1) && y<(16-1) && (c += target[y+1][x+1]*3);

			x>=(     2) && (c += target[y][x-2]);
			x<(width-2) && (c += target[y][x+2]);
			y>=(     2) && (c += target[y-2][x]);
			y<(16   -2) && (c += target[y+2][x]);

			x>=(     3) && (c += target[y][x-3]);
			x<(width-3) && (c += target[y][x+3]);
			y>=(     3) && (c += target[y-3][x]);
			y<(16   -3) && (c += target[y+3][x]);

			if(c >= 1200)
				dest[offs] = fgcolor;
			else if(c >= 30)
			{
				dest[offs] = (dest[offs] & 0x0F) | 0xC0;
			}
			else if(c > 0)
			{
				dest[offs] = (dest[offs] & 0x3F) | 0xC0;
			}
		}
}

