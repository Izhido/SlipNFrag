// r_image.c

#include "quakedef.h"
#include "r_local.h"

/*
=========================================================

TARGA LOADING

=========================================================
*/

typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;


/*
=============
LoadTGA
=============
*/
qboolean R_LoadTGA (const char *name, int start, qboolean extra, int mips, qboolean log_failure, byte **pic, int *piclen, int *width, int *height)
{
	int		columns, rows, numPixels;
	byte	*pixbuf;
	int		row, column;
	byte	*buf_p;
	byte	*buffer;
	TargaHeader		targa_header;
	byte			*targa_rgba;
	int				targa_len;
	int				realrow; //johnfitz -- fix for upside-down targas
	qboolean		upside_down; //johnfitz -- fix for upside-down targas
	byte tmp[2];

	//
	// load the file
	//
	static std::vector<byte> contents;
	buffer = COM_LoadFile (name, log_failure, contents);
	if (!buffer)
	{
		Con_DPrintf("R_LoadTGA: Bad tga file %s\n", name);
		return false;
	}

	buf_p = buffer;

	targa_header.id_length = *buf_p++;
	targa_header.colormap_type = *buf_p++;
	targa_header.image_type = *buf_p++;

	tmp[0] = buf_p[0];
	tmp[1] = buf_p[1];
	targa_header.colormap_index = LittleShort ( *((short *)tmp) );
	buf_p+=2;
	tmp[0] = buf_p[0];
	tmp[1] = buf_p[1];
	targa_header.colormap_length = LittleShort ( *((short *)tmp) );
	buf_p+=2;
	targa_header.colormap_size = *buf_p++;
	targa_header.x_origin = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.y_origin = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.width = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.height = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.pixel_size = *buf_p++;
	targa_header.attributes = *buf_p++;

	if (targa_header.image_type!=2
		&& targa_header.image_type!=10)
	{
		Con_Printf ("R_LoadTGA: %s - Only type 2 and 10 targa RGB images supported\n", name);
		return false;
	}

	if (targa_header.colormap_type !=0
		|| (targa_header.pixel_size!=32 && targa_header.pixel_size!=24))
	{
		Con_Printf ("R_LoadTGA: %s - Only 32 or 24 bit images supported (no colormaps)\n", name);
		return false;
	}

	columns = targa_header.width;
	rows = targa_header.height;
	numPixels = columns * rows;
	upside_down = !(targa_header.attributes & 0x20); //johnfitz -- fix for upside-down targas

	if (width)
		*width = columns;
	if (height)
		*height = rows;

	auto offset = start;
	if (extra)
	{
		offset += numPixels;
	}
	auto mipArea = numPixels*4;
	targa_len = offset + mipArea;
	mips--;
	while (mips > 0)
	{
		mipArea /= 4;
		targa_len += mipArea;
		mips--;
	}
	if ((*piclen) < targa_len)
	{
		delete[] (*pic);
		(*pic) = new byte[targa_len];
		(*piclen) = targa_len;
	}
	targa_rgba = (*pic);

	if (targa_header.id_length != 0)
		buf_p += targa_header.id_length;  // skip TARGA image comment

	if (targa_header.image_type==2)  // Uncompressed, RGB images
	{
		if (targa_header.pixel_size == 24)
		{
			for(row=rows-1; row>=0; row--)
			{
				//johnfitz -- fix for upside-down targas
				realrow = upside_down ? row : rows - 1 - row;
				pixbuf = targa_rgba + offset + realrow*columns*4;
				//johnfitz
				for(column=0; column<columns; column++)
				{
					auto blue = *buf_p++;
					auto green = *buf_p++;
					auto red = *buf_p++;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = 255;
				}
			}
		}
		else if (targa_header.pixel_size == 32)
		{
			auto span = columns * 4;
			for(row=rows-1; row>=0; row--)
			{
				//johnfitz -- fix for upside-down targas
				realrow = upside_down ? row : rows - 1 - row;
				pixbuf = targa_rgba + offset + realrow*span;
				//johnfitz
				for(column=0; column<columns; column++)
				{
					auto blue = *buf_p++;
					auto green = *buf_p++;
					auto red = *buf_p++;
					auto alpha = *buf_p++;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = alpha;
				}
			}
		}
	}
	else if (targa_header.image_type==10)   // Runlength encoded RGB images
	{
		if (targa_header.pixel_size == 24)
		{
			unsigned char red,green,blue,alphabyte,packetHeader,packetSize,j;
			for(row=rows-1; row>=0; row--) {
				//johnfitz -- fix for upside-down targas
				realrow = upside_down ? row : rows - 1 - row;
				pixbuf = targa_rgba + offset + realrow*columns*4;
				//johnfitz
				for(column=0; column<columns; ) {
					packetHeader= *buf_p++;
					packetSize = 1 + (packetHeader & 0x7f);
					if (packetHeader & 0x80) {        // run-length packet
						blue = *buf_p++;
						green = *buf_p++;
						red = *buf_p++;
						alphabyte = 255;

						for(j=0;j<packetSize;j++) {
							*pixbuf++=red;
							*pixbuf++=green;
							*pixbuf++=blue;
							*pixbuf++=alphabyte;
							column++;
							if (column==columns) { // run spans across rows
								column=0;
								if (row>0)
									row--;
								else
									goto breakOut24;
								//johnfitz -- fix for upside-down targas
								realrow = upside_down ? row : rows - 1 - row;
								pixbuf = targa_rgba + offset + realrow*columns*4;
								//johnfitz
							}
						}
					}
					else {                            // non run-length packet
						for(j=0;j<packetSize;j++) {
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = 255;
							column++;
							if (column==columns) { // pixel packet run spans across rows
								column=0;
								if (row>0)
									row--;
								else
									goto breakOut24;
								//johnfitz -- fix for upside-down targas
								realrow = upside_down ? row : rows - 1 - row;
								pixbuf = targa_rgba + offset + realrow*columns*4;
								//johnfitz
							}
						}
					}
				}
				breakOut24:;
			}
		}
		else if (targa_header.pixel_size == 32)
		{
			unsigned char red,green,blue,alphabyte,packetHeader,packetSize,j;
			for(row=rows-1; row>=0; row--) {
				//johnfitz -- fix for upside-down targas
				realrow = upside_down ? row : rows - 1 - row;
				pixbuf = targa_rgba + offset + realrow*columns*4;
				//johnfitz
				for(column=0; column<columns; ) {
					packetHeader= *buf_p++;
					packetSize = 1 + (packetHeader & 0x7f);
					if (packetHeader & 0x80) {        // run-length packet
						blue = *buf_p++;
						green = *buf_p++;
						red = *buf_p++;
						alphabyte = *buf_p++;

						for(j=0;j<packetSize;j++) {
							*pixbuf++=red;
							*pixbuf++=green;
							*pixbuf++=blue;
							*pixbuf++=alphabyte;
							column++;
							if (column==columns) { // run spans across rows
								column=0;
								if (row>0)
									row--;
								else
									goto breakOut32;
								//johnfitz -- fix for upside-down targas
								realrow = upside_down ? row : rows - 1 - row;
								pixbuf = targa_rgba + offset + realrow*columns*4;
								//johnfitz
							}
						}
					}
					else {                            // non run-length packet
						for(j=0;j<packetSize;j++) {
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							alphabyte = *buf_p++;
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = alphabyte;
							column++;
							if (column==columns) { // pixel packet run spans across rows
								column=0;
								if (row>0)
									row--;
								else
									goto breakOut32;
								//johnfitz -- fix for upside-down targas
								realrow = upside_down ? row : rows - 1 - row;
								pixbuf = targa_rgba + offset + realrow*columns*4;
								//johnfitz
							}
						}
					}
				}
				breakOut32:;
			}
		}
	}

	return true;
}


