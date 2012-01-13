/*
 * Copyright © 2009,2011 Rodolfo Ribeiro Gomes <rodolforg arr0ba gmail.com>
 *
 * SGGG image handler - Gimp plugin
 *   A pedido de OrakioRob (Roberto) <orakio arr0ba gazetadealgol.com.br>
 *
    This file is part of PSG1-toolkit.

    PSG1-toolkit is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PSG1-toolkit is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PSG1-toolkit.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <glib/gprintf.h>
#include <glib/gstrfuncs.h>


#if GTK_CHECK_VERSION (2, 6, 0)
#include <glib/gstdio.h>
#else
#include <stdio.h>
#define g_fopen fopen
#define g_fclose fclose
#endif


#define SAVE_PROC "file_sggg_save"
#define LOAD_PROC "file_sggg_load"

#define MAGIC_ID_STR "SGGG"
#define MAGIC_ID     0x53474747
#define FILE_TYPE "SGGG Image"

typedef struct {
	guint32 magic_id;
	guint32 version;
	guint16 width;
	guint16 height;
	guint32 pad2;
} SGGGHeader;

/*
typedef struct {
	guint8 r;
	guint8 g;
	guint8 b;
	guint8 a;
} SGGGColor;
*/
typedef guint32 SGGGColor;
#define COLOR_GET_A(x) (((x) & 0xff000000) >> 24)
#define COLOR_GET_B(x) (((x) & 0x00ff0000) >> 16)
#define COLOR_GET_G(x) (((x) & 0x0000ff00) >> 8)
#define COLOR_GET_R(x) (((x) & 0x000000ff) >> 0)

#define COLOR_RGBA(r, g, b, a) ( (((a)&0x0FF)<<24) | (((b)&0x0FF) << 16) | (((g)&0x0FF)<<8) | (((r)&0x0FF)))


typedef int SGGGSaveVals;

static const SGGGSaveVals defaults =
{
	0
};

static SGGGSaveVals sgggvals;

static void query (void);
static void run   (const gchar      *name,
						 gint              nparams,
						 const GimpParam  *param,
						 gint             *nreturn_vals,
						 GimpParam       **return_vals);

gboolean save_image(gchar* filename, gint32 image_id, gint32 drawable_id);
gint32 load_image(gchar* filename);

GimpPlugInInfo PLUG_IN_INFO =
{
	NULL,
	NULL,
	query,
	run
};

MAIN()

static void query (void)
{
	static const GimpParamDef load_args[] =
	{
		{ GIMP_PDB_INT32,  "run-mode",     "Interactive, non-interactive" },
		{ GIMP_PDB_STRING, "filename",     "The name of the file to load" },
		{ GIMP_PDB_STRING, "raw-filename", "The name of the file to load" }
	};
	static const GimpParamDef load_return_vals[] =
	{
		{ GIMP_PDB_IMAGE, "image", "Output image" }
	};

	static const GimpParamDef save_args[] =
	{
		{ GIMP_PDB_INT32,    "run_mode",        "Interactive, non-interactive" },
		{ GIMP_PDB_IMAGE,    "image",           "Image to save" },
		{ GIMP_PDB_DRAWABLE, "drawable",        "Drawable to save" },
		{ GIMP_PDB_STRING,   "filename",        "The name of the file to save the image in" },
		{ GIMP_PDB_STRING,   "raw_filename",    "The name entered by user" },
	};

	gimp_install_procedure (LOAD_PROC,
									"Loads files in SGGG file format",
									"Loads files in SGGG file format",
									"Rodolfo Gomes <rodolforg@gmail.com>",
									"Rodolfo Gomes <rodolforg@gmail.com>",
									"v0.2 2011",
									(FILE_TYPE),
									NULL,
									GIMP_PLUGIN,
									G_N_ELEMENTS (load_args),
									G_N_ELEMENTS (load_return_vals),
									load_args, load_return_vals
	);

	gimp_register_file_handler_mime (LOAD_PROC, "image/sggg");
	gimp_register_load_handler (LOAD_PROC, "sggg", "");
	gimp_register_magic_load_handler (LOAD_PROC,
												 "sggg", "", "0,string,SGGG");


	gimp_install_procedure (SAVE_PROC,
									"Saves files in SGGG image format.",
									"Saves files in SGGG image format.",
									"Rodolfo Gomes",
									"Rodolfo Gomes <rodolforg@gmail.com>",
									"v0.2 2011",
									"<Save>/SGGG",
									"RGB*, GRAY*, INDEXED*",
									GIMP_PLUGIN,
									G_N_ELEMENTS (save_args), 0,
									save_args, NULL);

	gimp_register_save_handler (SAVE_PROC, "sggg", "");
}

static void run (const gchar      *name,
			     gint              nparams,
			     const GimpParam  *param,
			     gint             *nreturn_vals,
			     GimpParam       **return_vals)
{

	static GimpParam  values[2];
	GimpPDBStatusType status = GIMP_PDB_EXECUTION_ERROR;
	GimpRunMode       run_mode = param[0].data.d_int32;
	GimpExportReturn  export = GIMP_EXPORT_CANCEL;

	gint32 image_id;
	GimpDrawable *drawable;
	gint32 drawable_id;

	/* Setting mandatory output values */
	*nreturn_vals = 1;
	*return_vals  = values;

	values[0].type = GIMP_PDB_STATUS;
	values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

	if (g_ascii_strcasecmp(name, LOAD_PROC)==0)
	{
		image_id = load_image(param[1].data.d_string);
		if (image_id > 0)
		{
      // Load success
			*nreturn_vals=2;
			values[1].type = GIMP_PDB_IMAGE;
			values[1].data.d_image = image_id;
			status = GIMP_PDB_SUCCESS;
		}
	}
	else if (g_ascii_strcasecmp(name,SAVE_PROC)==0)
	{
		gchar * filename;
		image_id = param[1].data.d_image;
		drawable = gimp_drawable_get (param[2].data.d_drawable);
		drawable_id = drawable->drawable_id;

		filename = param[3].data.d_string;

		// If not indexed, try to export it
		GimpImageBaseType image_type = gimp_image_base_type(image_id);
		gint num_colors = 0;
		status = GIMP_PDB_SUCCESS;
		if (image_type == GIMP_INDEXED)
		{
			gimp_ui_init(FILE_TYPE, FALSE);
			// Para achatar as camadas...
			if ( gimp_export_image(&image_id, &drawable_id, "SGGG", GIMP_EXPORT_CAN_HANDLE_INDEXED)
			      == GIMP_EXPORT_CANCEL)
				status = GIMP_PDB_CANCEL;
			else
				gimp_image_get_colormap(image_id, &num_colors);
		}
		if (image_type != GIMP_INDEXED || num_colors > 256)
		{
			gimp_ui_init(FILE_TYPE, FALSE);
			export = gimp_export_image(&image_id, &drawable_id, "SGGG", GIMP_EXPORT_CAN_HANDLE_INDEXED);
			switch (export)
			{
				case GIMP_EXPORT_CANCEL:
					status = GIMP_PDB_CANCEL;
					break;
				case GIMP_EXPORT_IGNORE:
					if (!gimp_image_convert_indexed(image_id, GIMP_FS_DITHER, GIMP_MAKE_PALETTE, 256, TRUE, FALSE, "none"))
					{
						g_message("Couldn't convert ignore");
						status = GIMP_PDB_EXECUTION_ERROR;
					}
					else
					{
						drawable_id = gimp_image_get_active_drawable(image_id);
					}
					break;
				case GIMP_EXPORT_EXPORT:
					break;
				default:
					g_message("Unknown export answer\n");
					status = GIMP_PDB_EXECUTION_ERROR;
					break;
			}
		}

		if (status == GIMP_PDB_SUCCESS)
		{
			if (save_image(filename, image_id, drawable_id))
			{
				status = GIMP_PDB_SUCCESS;
			}
			else
			{
				g_printf(FILE_TYPE ": not saved...\n");
				status = GIMP_PDB_EXECUTION_ERROR;
			}
			
			if (export == GIMP_EXPORT_EXPORT)
				gimp_image_delete(image_id);
		}
	}

	values[0].data.d_status = status;
}

/***************************************
**  Load
****************************************/

gint32 load_image(gchar* filename)
{
	FILE		  *f;
	gint32	       image, layer;
	GimpDrawable  *drawable;
	GimpPixelRgn   pixel_region;

#if G_BYTE_ORDER != G_LITTLE_ENDIAN
#warning System not Big Endian. Must check if it reads right
#endif
	SGGGHeader sggg_header;
	long filesize;
	SGGGColor palette[256];
	guchar * sggg_image_data;

	gimp_progress_init("Loading file...");

	// Open file
	f = g_fopen(filename,"rb");
	if (!f)
	{
		g_message (FILE_TYPE ": Can't open `%s'.", filename);
		return -1;
	}

	// Get file size
	if ( fseek(f, 0, SEEK_END) != 0)
	{
		perror(FILE_TYPE);
		g_message (FILE_TYPE ": Can't seek in `%s'.", filename);
		fclose(f);
		return -1;
	}
	filesize = ftell (f);
	if (filesize == -1)
	{
		perror(FILE_TYPE);
		g_message (FILE_TYPE ": Can't get size of `%s'.", filename);
		fclose(f);
		return -1;
	}
	if ( fseek(f, 0, SEEK_SET) != 0)
	{
		perror(FILE_TYPE);
		g_message (FILE_TYPE ": Can't seek in `%s'.", filename);
		fclose(f);
		return -1;
	}

	// Check if it's long enough
	if (filesize < sizeof(palette) + sizeof(SGGGHeader))
	{
		g_message (FILE_TYPE ": File is too short: `%s'.", filename);
		fclose(f);
		return -1;
	}

	// Read the file header
	if (fread(&sggg_header,1,sizeof(sggg_header),f) < sizeof(sggg_header))
	{
		g_message (FILE_TYPE ": Error reading SGGG header: `%s'", filename);
		fclose(f);
		return -1;
	}

	// Check if it's SGGG
	if (GUINT32_FROM_BE(sggg_header.magic_id) != MAGIC_ID)
	{
		fclose(f);
		g_message (FILE_TYPE ": Not a SGGG image `%s'.", filename);
		return -1;
	}

	// Check if it's a supported version
	if (GUINT_FROM_LE(sggg_header.version) != 0x01)
	{
		fclose(f);
		g_message (FILE_TYPE": version not supported `%d'.", sggg_header.version);
		return -1;
	}

	// Check again the file size
	if (filesize < sggg_header.width*sggg_header.height + sizeof(palette) + sizeof(SGGGHeader) )
	{
		// File is short, but it can still go on, just check the limits
		g_message (FILE_TYPE ": File is too short `%s'.", filename);
		sggg_header.height = ( filesize - (sizeof(palette) + sizeof(SGGGHeader)) ) / sggg_header.width;
	}

	if (filesize > sggg_header.width*sggg_header.height + sizeof(palette) + sizeof(SGGGHeader) )
		// File is too long, just ignore the trailing part
		g_message (FILE_TYPE ": File is too long `%s'.", filename);

	// Read the palette
	fread( &palette, sizeof(SGGGColor), 256, f);

	// Read the image data
	sggg_image_data = (guchar*) g_malloc(sggg_header.width * sggg_header.height); // byte & alpha
	if (sggg_image_data == NULL)
	{
		fclose(f);
		g_message (FILE_TYPE": Couldn't allocate data for `%s'.", filename);
		return -1;
	}
	//printf("tamanho = %d : restam %d \n", sggg_header.width * sggg_header.height, filesize - ftell(f));
	fread( sggg_image_data, sggg_header.width * sggg_header.height, 1, f);

	fclose(f);



  // Create a GIMP image
	image = gimp_image_new (sggg_header.width, sggg_header.height, GIMP_INDEXED);
	if (image == -1)
	{
		g_free(sggg_image_data);
		g_message ("Can't allocate new image.\n");
		return -1;
	}

	gimp_image_set_filename (image, filename);

	// Convert palette
		gint n;
		guint16 color;
		guchar * colormap = NULL;

		colormap = g_malloc(256*3);
		if (!colormap)
		{
			g_message(FILE_TYPE ": Can't allocate data for colormap!");
			gimp_image_delete(image);
			g_free(sggg_image_data);
			return -1;
		}

		for (n = 0; n < 256; n++)
		{
			if (COLOR_GET_A(palette[n]) != 0x80)
			{
				if (n != 0)
				{
					g_message(FILE_TYPE ": Não é só a primeira cor da paleta que é de transparência...");
					printf("palette[%d] %X %X %X %X \n", n, COLOR_GET_R(palette[n]), COLOR_GET_G(palette[n]), COLOR_GET_B(palette[n]), COLOR_GET_A(palette[n]) );
				}

//				palette[n] = COLOR_RGBA(255, 255, 255,  255);
//				palette[n].r = 0xff;
//				palette[n].g = 0xff;
//				palette[n].b = 0xff;
			}
			
			// Conversão estranha...
			gint palette_idx;
			switch ((n/8)%4)
			{
				case 1:
					palette_idx = n + 8;
					break;
				case 2:
					palette_idx = n - 8;
					break;
				default:
					palette_idx = n;
					break;
			}
			
			colormap[palette_idx*3+0] = COLOR_GET_R(palette[n]);//palette[n].b;//((color >> 0) & 0x1F) << 3;
			colormap[palette_idx*3+1] = COLOR_GET_G(palette[n]);// ((color >> 5) & 0x1F) << 3;
			colormap[palette_idx*3+2] = COLOR_GET_B(palette[n]);// ((color >> 10) & 0x1F) << 3;
		}
		gimp_image_set_colormap(image, colormap, 256);
		g_free(colormap);


  // Create the "background" layer to hold the image...
	layer = gimp_layer_new (image, "Background", sggg_header.width, sggg_header.height,
									 GIMP_INDEXED_IMAGE ,
									100, GIMP_NORMAL_MODE);

	if (! gimp_image_add_layer (image, layer, 0) )
	{
		g_message(FILE_TYPE ": Can't add layer");
		gimp_image_delete(image);
		g_free(sggg_image_data);
		return -1;
	}

  // Get the drawable and set the pixel region for our load...
	drawable = gimp_drawable_get (layer);
	if (drawable == NULL)
	{
		g_message(FILE_TYPE ": Can't get drawable!");
		gimp_image_delete(image);
		g_free(sggg_image_data);
		return -1;
	}

	gimp_pixel_rgn_init (&pixel_region, drawable, 0, 0, sggg_header.width,
								 drawable->height, TRUE, FALSE);

	guint r;

	for (r = 0; r < sggg_header.width; r+=512)
	{
		gint region_width = (r+512 > sggg_header.width)? (sggg_header.width - r) : 512;
		gimp_pixel_rgn_set_rect(&pixel_region, &sggg_image_data[r*sggg_header.height], r, 0, region_width, sggg_header.height);
	}

  // temporary
	g_free(sggg_image_data);

	gimp_drawable_flush (drawable);
	gimp_drawable_detach (drawable);

	return image;
}

/*************************************************
**  Save
**************************************************/

gboolean save_image(gchar* filename, gint32 image_id, gint32 drawable_id)
{
	FILE *f;
	SGGGHeader sggg_header;
	SGGGColor palette[256];
	gint16 width =  gimp_image_width (image_id);
	gint16 height =  gimp_image_height (image_id);
	gint channels;
	guchar *img_buffer;
	GimpPixelRgn rgn;
	int i=0, j=0, img_x=0;

	GimpDrawable * drawable = gimp_drawable_get (drawable_id);

	if (!drawable)
		return FALSE;

	f = g_fopen(filename,"wb");
	if (!f)
	{
		g_message (FILE_TYPE ": Can't open `%s'.", filename);
 		return FALSE;
	}


	sggg_header.magic_id = GUINT_TO_BE(MAGIC_ID);
	sggg_header.version = GUINT_TO_LE(0x0001);
	sggg_header.width = GUINT_TO_LE(width);
	sggg_header.height = GUINT_TO_LE(height);

	if (fwrite(&sggg_header, sizeof(sggg_header), 1, f) == 0)
	{
		g_message(FILE_TYPE ": Error while writing on file [header]");
		fclose(f);
		return FALSE;
	}

	// Save palette
	gint colormap_size = 0;
	const guchar *colormap = gimp_image_get_colormap(image_id, &colormap_size);
	gint n;
	for (n = 0; n < colormap_size; n++)
	{
		guint32 color = 0;
		guchar alpha = 0x80;
		// FIXME: Transparência forçada na cor #0
		if (n == 0)
			alpha = 0x00;
		// Conversão estranha inversa...
		gint palette_idx;
		switch ((n/8)%4)
		{
			case 1:
				palette_idx = n + 8;
				break;
			case 2:
				palette_idx = n - 8;
				break;
			default:
				palette_idx = n;
				break;
		}
		
		color = COLOR_RGBA(colormap[palette_idx*3+0], colormap[palette_idx*3+1], colormap[palette_idx*3+2], alpha);
		color = GUINT_TO_LE(color);
		fwrite(&color, 1, 4, f);
	}
	if (colormap_size < 256)
	{
		guint32 dummy_color = COLOR_RGBA(0,0,0,0x80) ;
		dummy_color = GUINT_TO_LE(color);
		for (n = colormap_size; n < 256; n++)
			fwrite(&dummy_color, 4, 1, f);
	}
		

	// Image data finally
	channels = 1;
	if (gimp_drawable_has_alpha (drawable_id))
		channels ++;

	img_buffer = g_new(guchar, 512*height);

	gimp_pixel_rgn_init(&rgn, drawable, 0, 0, width, height, FALSE, FALSE);

	int r;
	for (r = 0; r < width; r+= 512)
	{
		gint region_width = (width-r <512)? width - r : 512;
		gimp_pixel_rgn_get_rect(&rgn, img_buffer, r, 0, region_width, height);
		fwrite(img_buffer, region_width* height, 1, f);
	}

	fclose(f);

	g_free(img_buffer);

	return TRUE;
}
