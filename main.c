#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "png.h"
#include "pngstruct.h"

// Global variable setup
int width, height, x, y;
png_byte color_type;
png_byte bit_depth;
png_byte interlace_type;
png_byte compression_type;
png_byte filter_method;
png_bytep *row_pointers;
int num_rows;
png_size_t rowbytes;





// Error handler
void log_error(char *err_str)
{
	fprintf(stderr, "[-] ERROR: %s\n", err_str);
	exit(EXIT_FAILURE);
}

// Info printing
void log_info(char *info_str)
{
	fprintf(stderr, "[*] %s\n", info_str);
}

// Read timestamp counter
unsigned long long rdtsc(){
	unsigned int lo,hi;
	__asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
	return ((unsigned long long)hi << 32) | lo;
}


// Read file into memory
void read_file(char *filename, char adam7_mod, int stop_pass, int stop_early)
{
	// Verify that the file can be read
	FILE *fp = fopen(filename, "rb");
	if (!fp)
		log_error("[READ] Couldn't open the file.");

	// Allocate and initialize png_struct
	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png)
		log_error("[READ] Failed to create PNG struct.");

	// Allocate and initialize png_info
	png_infop info = png_create_info_struct(png);
	if (!info)
	{
		png_destroy_read_struct(&png, (png_infopp)NULL, (png_infopp)NULL);
		log_error("[READ] Failed to create INFO struct.");
	}

	// Set up error handler
	if (setjmp(png_jmpbuf(png)))
	{
		png_destroy_read_struct(&png, &info, NULL);
		fclose(fp);
		log_error("[READ] Couldn't initialize I/O.");
	}

	// Initialize the libpng I/O subroutines
	png_init_io(png, fp);

	// Process all chunks preceding the image data
	png_read_info(png, info);

	// Get IHDR information (16-bit failsafe)
	width				= png_get_image_width(png, info);
	height				= png_get_image_height(png, info);
	bit_depth			= png_get_bit_depth(png, info);
	color_type			= png_get_color_type(png, info);
	interlace_type		= png_get_interlace_type(png, info);
	compression_type	= png_get_compression_type(png, info);
	filter_method		= png_get_filter_type(png, info);

	// Ensure all images are at most 8 bit RGB
	if (bit_depth == 16)
		png_set_strip_16(png);

	// Convert paletted images to RGB
	if(color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png);

	// Ensure all images are at least 8 bit RGB
	if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(png);

	// Create transparency from tRNS info
	if(png_get_valid(png, info, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png);

	// These color_type don't have an alpha channel then fill it with 0xff.
	if(color_type == PNG_COLOR_TYPE_RGB ||
		color_type == PNG_COLOR_TYPE_GRAY ||
		color_type == PNG_COLOR_TYPE_PALETTE)
		//png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
		png_set_add_alpha(png, 0xFF, PNG_FILLER_AFTER);

	if(color_type == PNG_COLOR_TYPE_GRAY ||
		color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png);

	// Set interlace handling if interlaced
	int pass, num_pass;
	if (adam7_mod || png->interlaced)
		num_pass = png_set_interlace_handling(png);
	else
		num_pass = 1;

	// Update the info struct with the new data
	png_read_update_info(png, info);

	// Allocate space for row pointers
	row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);

	// Allocate space for row bytes
	rowbytes = png_get_rowbytes(png, info);
	for (y = 0; y < height; y++)
		row_pointers[y] = (png_byte*)malloc(rowbytes);

	// Stop at the designated pass if -i and -p were specified
	if (adam7_mod)
		num_pass = stop_pass;

	// Stop at a random spot in the pass if -s was also specified
	int stop_row;
	if (stop_early)
		stop_row = rand() % height;

	// Read the image data
	for (pass = 0; pass < num_pass; pass++)
	{
		int last_pass = num_pass - 1;
		for (y = 0; y < height; y++)
		{
			if (stop_early && pass == last_pass && y == stop_row)
				break;
			else
				png_read_rows(png, NULL, &row_pointers[y], 1);
		}
	}

	// Finish reading the image data
	//png_read_image(png, row_pointers);
	png_read_end(png, info);

	// Free the libpng-allocated memory
	png_destroy_read_struct(&png, &info, NULL);

	// Close the file handle
	fclose(fp);

	// Print status message
	log_info("[READ] File read into memory without errors.\n");
}


// Write file from memory
void write_file(char *filename, int algorithm, int decoder, int interlace, int mask, int mask_avg, int force_alpha)
{
	FILE *fp;
	png_structp png;
	png_infop info;

	// Open the file
	fp = fopen(filename, "wb");
	if (!fp)
		log_error("[WRITE] Couldn't open the file.");

	// Create and initialize the PNG struct
	png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png)
	{
		fclose(fp);
		log_error("[WRITE] Failed to create INFO struct.");
	}

	// Initialize image information data
	info = png_create_info_struct(png);
	if (!info)
	{
		fclose(fp);
		png_destroy_write_struct(&png, NULL);
		log_error("[WRITE] Failed to create INFO struct.");
	}

	// Set up error handler
	if (setjmp(png_jmpbuf(png)))
	{
		fclose(fp);
		png_destroy_write_struct(&png, &info);
		log_error("[WRITE] Couldn't initialize I/O.");
	}

	// I/O initialization
	png_init_io(png, fp);

	//png_set_filter(png, PNG_FILTER_TYPE_BASE, PNG_ALL_FILTERS);

	// Force alpha channel if specified
	if (force_alpha == 1) {
		if (color_type == PNG_COLOR_TYPE_RGB) {
			color_type = PNG_COLOR_TYPE_RGBA;
        }
    }
	// Remove alpha channel if specified
	else if (force_alpha == 0) {
		if (color_type == PNG_COLOR_TYPE_RGBA) {
			color_type = PNG_COLOR_TYPE_RGB;
        }
    }

	png_set_IHDR(
		png,
		info,
		width,
		height,
		8,
		color_type,
		interlace,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);

	png_write_info(png, info);

	// Strip 4/8 bpp images down to 3/6 bpp
	if(color_type == PNG_COLOR_TYPE_RGB ||
		color_type == PNG_COLOR_TYPE_GRAY ||
		color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_filler(png, 0xFF, PNG_FILLER_AFTER);



	// START DEBUG STUFF
	int pass, num_pass;

	if (interlace == PNG_INTERLACE_ADAM7)
		num_pass = png_set_interlace_handling(png);
	else
		num_pass = 1;

	// Don't force filtering if not specified
	if (algorithm == 255) {
		png->force_filter = 0;
		decoder = 255;
	}
	else {
		png->force_filter = 1;
	}

	// Loop through passes
	for (pass = 0; pass < num_pass; pass++)
	{
		// Loop through image
		for (x = 0; x < height; x++)
		{
			png->interlaced = interlace;
			// Algorithm to apply to the pixel bytes
			png->filter_algorithm = algorithm;
			// Algorithm to force decoding through
			png->filter_decoder = decoder;
			// Value to mask bytes with
			png->byte_mask = mask;
			// Additional mask for Average and Paeth
			png->byte_mask_avg = mask_avg;
			// Write the scanline to the output file
			png_write_row(png, row_pointers[x]);
		}
	}


// END DEBUG STUFF



	//png_write_image(png, row_pointers);
	png_write_end(png, NULL);

	for(y = 0; y < height; y++)
		free(row_pointers[y]);
	free(row_pointers);

	if (algorithm > 0)
		png_destroy_write_struct(&png, &info);

	fclose(fp);

	log_info("[WRITE] File written from memory without errors.\n");
}


// Show program help
int show_help(char* filename)
{
	// Print error
	fprintf(stderr, "Usage: %s [options] <infile> <outfile> \n", filename);
	fprintf(stderr, "\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "    -a <filter>   Filter to use for writing row data\n");
	fprintf(stderr, "    -d <filter>   Filter to use for decoding row data\n");
	fprintf(stderr, "    -h            Show this help message\n");
	fprintf(stderr, "    -i            Interlace the image data\n");
	fprintf(stderr, "    -p <num>      Interlace pass to stop at (1-7)\n");
	fprintf(stderr, "    -s            Stop the interlace process mid-pass\n");
	fprintf(stderr, "    -x <byte>     Byte mask to modify filter with\n");
	fprintf(stderr, "    -y <byte>     Byte mask for AVG / PAETH output\n");
	fprintf(stderr, "    -z <num>      Force alpha channel off (0) or on (1)\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Filters:\n");
	fprintf(stderr, "    0:   None\n");
	fprintf(stderr, "    1:   Sub\n");
	fprintf(stderr, "    2:   Up\n");
	fprintf(stderr, "    3:   Average\n");
	fprintf(stderr, "    4:   Paeth\n");
	fprintf(stderr, "    5:   Sub (Modified)\n");
	fprintf(stderr, "    6:   Up (Modified)\n");
	fprintf(stderr, "    7:   Average (Modified)\n");
	fprintf(stderr, "    8:   Paeth (Modified)\n");
    fprintf(stderr, "    255: Random (Output only)\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Examples:\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    Write with Sub, read with Up:\n", filename);
    fprintf(stderr, "        %s -a 1 -d 2 in.png out.png \n", filename);
    fprintf(stderr, "\n");
    fprintf(stderr, "    Write with Up (Modified), AND each pixel with 0xFE, read with Up:\n", filename);
    fprintf(stderr, "        %s -a 6 -d 2 -x 0xFE in.png out.png \n", filename);
    fprintf(stderr, "\n");
    fprintf(stderr, "    Write with Up, read with Paeth, interlace up to pass 2 and stop mid-pass:\n", filename);
    fprintf(stderr, "        %s -a 2 -d 4 -i -p 2 -s in.png out.png \n", filename);
    fprintf(stderr, "\n");
    fprintf(stderr, "    Write normally, read randomly:\n", filename);
    fprintf(stderr, "        %s -d 255 in.png out.png \n\n", filename);
	exit(EXIT_FAILURE);
}


// Program start
int main(int argc, char* argv[])
{
	fprintf(stderr, "\nGlitch Suite - PNG\n\n");

	// Get filename from path
	char *token, *filename;
	token = strtok(argv[0], "\\");
	while (token != NULL) {
		filename = token;
		token = strtok(NULL, "\\");
	}

	// Show help if bad arg count
	if (argc < 3) {
		show_help(filename);
	}

	char *in_file = "";
	char *out_file = "";
	int algorithm = 1;
	int decoder = 1;
	int interlace = 0;
	int pass_num = 7;
	int stop_early = 0;
	char *mask = 255;
	char *mask_avg = 255;
	int force_alpha = 255;
	int opt_count;

	while (optind < argc)
	{
		if ((opt_count = getopt(argc, argv, "a:d:hip:sx:y:z:")) != -1)
		{
			switch(opt_count)
			{
			case 'a':
				algorithm = atoi(optarg);
				break;
			case 'd':
				decoder = atoi(optarg);
				break;
			case 'h':
				show_help(filename);
				break;
			case 'i':
				interlace = 1;
				break;
			case 'p':
				pass_num = atoi(optarg);
				break;
			case 's':
				stop_early = 1;
				break;
			case 'x':
				mask = strtol(optarg, NULL, 16);
				break;
			case 'y':
				mask_avg = strtol(optarg, NULL, 16);
				break;
			case 'z':
				force_alpha = atoi(optarg);
				break;
			case '?':
				if (optopt == 'a' || optopt == 'd' || optopt == 'p' || optopt == 'x' || optopt == 'y' || optopt == 'z')
					fprintf(stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint(optopt))
					fprintf(stderr, "Unknown option '-%c'.\n", optopt);
				else
					fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
				exit(EXIT_FAILURE);
			default:
				abort();
			}
		}
		else
		{
			if (!strlen(in_file))
			{
				in_file = argv[optind++];
				out_file = argv[optind++];
			}
		}
	}

	// Set the random seed based on the current timestamp
	srand(time(NULL) * rdtsc());

	// Read the file into memory
	read_file(in_file, 0, 0, 0);

	// Write the file to disk
	write_file(out_file, algorithm, decoder, interlace, mask, mask_avg, force_alpha);

	// Re-read and re-write the file as instructed
	if (interlace)
	{
		read_file(out_file, 1, pass_num, stop_early);
		write_file(out_file, 1, 1, 1, "0xFF", "0xFF", force_alpha);
	}
}
