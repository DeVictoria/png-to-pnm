#include "return_codes.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(ZLIB)
#include <zlib.h>
#elif defined(LIBDEFLATE)
#include <libdeflate.h>
#elif defined(ISAL)
#error "ISAL unsupported"
#else
#error "unsupported library"
#endif

#define PLTE 1347179589
#define IDAT 1229209940
#define IEND 1229278788
#define BKGD 1649100612
#define TRNS 1951551059

struct Ihdr
{
	long long width;
	long long height;
	int depth;
	int color_type;
};

struct Plte
{
	long long size;
	unsigned char *source;
};

struct Idat
{
	long long size;
	unsigned char *source;
};
struct Bkgd
{
	bool have_bkgd;
	unsigned char background[3];
};
struct Trns
{
	bool have_trns;
	unsigned char background[3];
	unsigned char *point_background;
};

void get_message(int code)
{
	switch (code)
	{
	case 0:
		fprintf(stderr, "The operation completed successfully");
		break;
	case 1:
		fprintf(stderr, "File can't be opened");
		break;
	case 2:
		fprintf(stderr, "Not enough memory, memory allocation failed");
		break;
	case 3:
		fprintf(stderr, "The data is invalid");
		break;
	case 4:
		fprintf(stderr, "The parameter or number of parameters (argv) is incorrect");
		break;
	case 5:
		fprintf(stderr, "Unsupported functionality");
		break;
	default:
		fprintf(stderr, "Unknown error");
	}
}

int get_format(struct Plte plte)
{
	for (int i = 0; i < plte.size; i += 3)
	{
		if (plte.source[i] != plte.source[i + 1] || plte.source[i] != plte.source[i + 2])
			return 6;
	}
	return 5;
}

int write_in_file(struct Ihdr ihdr, struct Plte plte, struct Bkgd bkgd, struct Trns trns, unsigned char *decompress_source, char *File)
{
	FILE *pnm = fopen(File, "wb");
	if (pnm == NULL)
	{
		return ERROR_CANNOT_OPEN_FILE;
	}

	switch (ihdr.color_type)
	{
	case 0:
		fprintf(pnm, "P5\n");
		fprintf(pnm, "%lli %lli\n", ihdr.width, ihdr.height);
		fprintf(pnm, "255\n");
		for (int i = 0; i < ihdr.height; i++)
		{
			for (int j = 1; j < ihdr.width + 1; j++)
			{
				if (!trns.have_trns || (trns.background[0] != decompress_source[i * (ihdr.width + 1)]))
				{
					if (fwrite(decompress_source + i * (ihdr.width + 1) + j, sizeof(unsigned char), 1, pnm) != 1)
					{
						fclose(pnm);
						return ERROR_UNKNOWN;
					}
				}
				else
				{
					if (fwrite(bkgd.background, sizeof(unsigned char), 1, pnm) != 1)
					{
						fclose(pnm);
						return ERROR_UNKNOWN;
					}
				}
			}
		}
		break;
	case 2:
		fprintf(pnm, "P6\n");
		fprintf(pnm, "%lli %lli\n", ihdr.width, ihdr.height);
		fprintf(pnm, "255\n");
		for (int i = 0; i < ihdr.height; i++)
		{
			for (int j = 1; j < ihdr.width * 3 + 1; j += 3)
			{
				for (int l = 0; l < 3; l++)
				{
					if (!trns.have_trns || (trns.background[l] != plte.source[decompress_source[i * (ihdr.width * 3 + 1) + j + l]]))
					{
						if (plte.size > 0)
						{
							if (fwrite(plte.source + decompress_source[i * (ihdr.width * 3 + 1) + j + l], sizeof(unsigned char), 1, pnm) != 1)
							{
								fclose(pnm);
								return ERROR_UNKNOWN;
							}
						}
						else
						{
							if (fwrite(decompress_source + i * (ihdr.width * 3 + 1) + j + l, sizeof(unsigned char), 1, pnm) != 1)
							{
								fclose(pnm);
								return ERROR_UNKNOWN;
							}
						}
					}
					else
					{
						if (fwrite(bkgd.background + l, sizeof(unsigned char), 1, pnm) != 1)
						{
							fclose(pnm);
							return ERROR_UNKNOWN;
						}
					}
				}
			}
		}
		break;
	case 3:
		fprintf(pnm, "P%i\n", get_format(plte));
		fprintf(pnm, "%lli %lli\n", ihdr.width, ihdr.height);
		fprintf(pnm, "255\n");
		int count = get_format(plte) == 6 ? 3 : 1;
		for (int i = 0; i < ihdr.height; i++)
		{
			for (int j = 0; j < ihdr.width; j++)
			{
				for (int l = 0; l < count; l++)
				{
					if (!trns.have_trns || (trns.point_background[decompress_source[i * (ihdr.width + 1) + j] * 3 + l] != 0))
					{
						if (fwrite(plte.source + decompress_source[i * (ihdr.width + 1) + j] * 3 + l, sizeof(unsigned char), 1, pnm) != 1)
						{
							fclose(pnm);
							return ERROR_UNKNOWN;
						}
					}
					else
					{
						if (fwrite(plte.source + bkgd.background[0], sizeof(unsigned char), 1, pnm) != 1)
						{
							fclose(pnm);
							return ERROR_UNKNOWN;
						}
					}
				}
			}
		}
		break;
	case 4:
		fprintf(pnm, "P5\n");
		fprintf(pnm, "%lli %lli\n", ihdr.width, ihdr.height);
		fprintf(pnm, "255\n");
		for (int i = 0; i < ihdr.height; i++)
		{
			for (int j = 0; j < ihdr.width * 2; j += 2)
			{
				if (decompress_source[i * (ihdr.width * 2 + 1) + j + 2] != 0)
				{
					if (fwrite(decompress_source + i * (ihdr.width * 2 + 1) + j + 1, sizeof(unsigned char), 1, pnm) != 1)
					{
						fclose(pnm);
						return ERROR_UNKNOWN;
					}
				}
				else
				{
					if (fwrite(bkgd.background, sizeof(unsigned char), 1, pnm) != 1)
					{
						fclose(pnm);
						return ERROR_UNKNOWN;
					}
				}
			}
		}
		break;
	case 6:
		fprintf(pnm, "P6\n");
		fprintf(pnm, "%lli %lli\n", ihdr.width, ihdr.height);
		fprintf(pnm, "255\n");
		for (int i = 0; i < ihdr.height; i++)
		{
			for (int j = 0; j < ihdr.width * 4; j += 4)
			{
				if (decompress_source[i * (ihdr.width * 4 + 1) + j + 4] != 0)
				{
					if (plte.size == 0)
					{
						if (fwrite(decompress_source + i * (ihdr.width * 4 + 1) + j + 1, sizeof(unsigned char), 3, pnm) != 3)
						{
							fclose(pnm);
							return ERROR_UNKNOWN;
						}
					}
					else
					{
						if (fwrite(plte.source + decompress_source[i * (ihdr.width * 4 + 1) + j + 1], sizeof(unsigned char), 3, pnm) != 3)
						{
							fclose(pnm);
							return ERROR_UNKNOWN;
						}
					}
				}
				else
				{
					if (fwrite(bkgd.background, sizeof(unsigned char), 3, pnm) != 3)
					{
						fclose(pnm);
						return ERROR_UNKNOWN;
					}
				}
			}
		}
		break;
	default:
		fclose(pnm);
		return ERROR_DATA_INVALID;
	}
	fclose(pnm);
	return SUCCESS;
}

long long get_long_from_file(int CountByte, FILE *file)
{
	long long ans = 0;
	for (int i = 0; i < CountByte; i++)
	{
		int ff = fgetc(file);
		ans += ff * (long long)pow(2, 8 * (CountByte - 1 - i));
	}
	return ans;
}

int get_IHDR(struct Ihdr *ihdr, FILE *png)
{
	int Signature[16] = { 137, 80, 78, 71, 13, 10, 26, 10, 0, 0, 0, 13, 73, 72, 68, 82 };

	for (int i = 0; i < 16; i++)
		if (Signature[i] != fgetc(png))
		{
			return ERROR_DATA_INVALID;
		}
	ihdr->width = get_long_from_file(4, png);
	ihdr->height = get_long_from_file(4, png);
	ihdr->depth = fgetc(png);
	ihdr->color_type = fgetc(png);
	int compression = fgetc(png);
	int filter = fgetc(png);
	int interlace = fgetc(png);
	get_long_from_file(4, png);
	if (compression != 0 || ihdr->width == 0 || ihdr->height == 0 || filter != 0 || ihdr->depth != 8 ||
		(ihdr->color_type != 0 && ihdr->color_type != 2 && ihdr->color_type != 3 && ihdr->color_type != 4 && ihdr->color_type != 6) ||
		(interlace != 0 && interlace != 1))
		return ERROR_DATA_INVALID;
	return SUCCESS;
}

int read_chunks(struct Ihdr *ihdr, struct Idat *idat, struct Plte *plte, struct Bkgd *bkgd, struct Trns *trns, FILE *png)
{
	int return_code = get_IHDR(ihdr, png);
	if (return_code != SUCCESS)
	{
		return return_code;
	}
	idat->size = 0;
	plte->size = 0;
	long long size = get_long_from_file(4, png);
	long long name = get_long_from_file(4, png);
	while (name != IEND && !feof(png))
	{
		switch (name)
		{
		case IDAT:
			idat->source = realloc(idat->source, idat->size + size);
			if (idat->source == NULL)
			{
				return ERROR_OUT_OF_MEMORY;
			}
			if (fread(idat->source + idat->size, sizeof(unsigned char), size, png) != size)
				return ERROR_UNKNOWN;
			idat->size += size;
			break;
		case PLTE:
			plte->size = size;
			if (((ihdr->color_type == 0) || (ihdr->color_type == 4)) && size > 0)
				return ERROR_DATA_INVALID;
			plte->source = malloc(sizeof(unsigned char) * size);
			if (plte->source == NULL)
			{
				return ERROR_OUT_OF_MEMORY;
			}
			if (fread(plte->source, sizeof(unsigned char), size, png) != size)
				return ERROR_UNKNOWN;
			if (plte->size % 3 != 0)
			{
				return ERROR_DATA_INVALID;
			}
			break;
		case BKGD:
			bkgd->have_bkgd = true;
			if ((size == 1) && (ihdr->color_type == 3))
			{
				long long ischar = get_long_from_file(1, png);
				bkgd->background[0] = (ischar < 256 ? ischar : 0);
			}
			else if ((size == 2 || size == 6) &&
					 (ihdr->color_type == 0 || ihdr->color_type == 2 || ihdr->color_type == 4 || ihdr->color_type == 6))
			{
				for (int i = 0; i < size / 2; i++)
				{
					long long ischar = get_long_from_file(2, png);
					bkgd->background[i] = (ischar < 256 ? ischar : 0);
				}
			}
			else
				return ERROR_DATA_INVALID;
			break;
		case TRNS:
			trns->have_trns = true;
			if ((size > 1) && (ihdr->color_type == 0))
			{
				trns->background[0] = get_long_from_file(2, png);
				if (fseek(png, (long)(size - 2), SEEK_CUR) != 0)
					return ERROR_UNKNOWN;
			}
			else if ((size > 5) && (ihdr->color_type == 2))
			{
				for (int i = 0; i < 3; i++)
				{
					trns->background[i] = get_long_from_file(2, png);
				}
				if (fseek(png, (long)(size - 6), SEEK_CUR) != 0)
					return ERROR_UNKNOWN;
			}
			else if ((size > 0) && (ihdr->color_type == 3))
			{
				trns->point_background = malloc(sizeof(unsigned char) * plte->size);
				if (trns->point_background == NULL)
				{
					return ERROR_OUT_OF_MEMORY;
				}
				if (fread(trns->point_background, sizeof(unsigned char), size, png) != size)
					return ERROR_UNKNOWN;
				if (size > plte->size)
					return ERROR_DATA_INVALID;
				for (long long i = size; i < plte->size; i++)
					trns->point_background[i] = 255;
			}
			else if ((ihdr->color_type == 4) || (ihdr->color_type == 6))
			{
				if (fseek(png, (long)size, SEEK_CUR) != 0)
					return ERROR_UNKNOWN;
			}
			else
			{
				return ERROR_DATA_INVALID;
			}
			break;
		default:
			if (fseek(png, (long)size, SEEK_CUR) != 0)
				return ERROR_UNKNOWN;
			break;
		}
		get_long_from_file(4, png);
		size = get_long_from_file(4, png);
		name = get_long_from_file(4, png);
	}
	if (feof(png))
	{
		return ERROR_DATA_INVALID;
	}
	return SUCCESS;
}

int do_decompress(unsigned char *decompress_source, unsigned long *decompress_len, unsigned char *source, unsigned long size)
{
#if defined(ZLIB)
	int return_code = uncompress(decompress_source, decompress_len, source, size);
	if (return_code == Z_MEM_ERROR)
		return ERROR_OUT_OF_MEMORY;
	else if ((return_code == Z_BUF_ERROR) || (return_code == Z_DATA_ERROR))
	{
		return ERROR_DATA_INVALID;
	}
	return SUCCESS;
#elif defined(LIBDEFLATE)
	struct libdeflate_decompressor *decompressor = libdeflate_alloc_decompressor();
	if (decompressor == NULL)
		return ERROR_OUT_OF_MEMORY;
	size_t actual_out_nbytes;
	if (libdeflate_zlib_decompress(decompressor, source, size, decompress_source, *decompress_len, &actual_out_nbytes) != LIBDEFLATE_SUCCESS)
	{
		libdeflate_free_decompressor(decompressor);
		return ERROR_DATA_INVALID;
	}
	libdeflate_free_decompressor(decompressor);
	return SUCCESS;
#elif defined(ISAL)
#error "ISAL not supported"
#else
#error "unsupported library"
#endif
}
int get_background_color(int argc, char **argv, struct Bkgd *bkgd)
{
	if (argc == 4)
	{
		int i = 0;
		int step = 0;
		int number = 0;
		int count_number = 0;
		while (argv[3][i] != '\0')
		{
			if (count_number > 3)
				return ERROR_PARAMETER_INVALID;
			if (argv[3][i] == 32)
			{
				i++;
				step = 0;
				bkgd->background[count_number] = (number < 256 ? number : 0);
				number = 0;
				count_number++;
			}
			else if ((argv[3][i] > 47) && (argv[3][i] < 58))
			{
				number = number * 10 + (argv[3][i] - 48);
				i++;
			}
			else
				return ERROR_PARAMETER_INVALID;
		}
		if (count_number == 2)
			bkgd->background[count_number] = (number < 256 ? number : 0);
	}
	return SUCCESS;
}
void do_sub_filter(unsigned char *source, long long int cur_height, long long int width, int byte_on_pixel)
{
	for (int i = byte_on_pixel + 1; i < width; i++)
		source[cur_height * width + i] = (source[cur_height * width + i] + source[cur_height * width + i - byte_on_pixel]) % 256;
}

void do_up_filter(unsigned char *source, long long int cur_height, long long int width)
{
	for (int i = 1; i < width; i++)
		source[cur_height * width + i] = (source[cur_height * width + i] + source[(cur_height - 1) * width + i]) % 256;
}

void do_average_filter(unsigned char *source, long long int cur_height, long long int width, int byte_on_pixel)
{
	for (int i = 1; i < width; i++)
	{
		unsigned char a = i <= byte_on_pixel ? 0 : source[cur_height * width + i - byte_on_pixel];
		unsigned char b = cur_height == 0 ? 0 : source[(cur_height - 1) * width + i];
		source[i] = (source[cur_height * width + i] + (a + b) / 2) % 256;
	}
}

void do_paeth_filter(unsigned char *source, long long int cur_height, long long int width, int byte_on_pixel)
{
	for (int i = 1; i < width; i++)
	{
		unsigned char a = i <= byte_on_pixel ? 0 : source[cur_height * width + i - byte_on_pixel];
		unsigned char b = cur_height == 0 ? 0 : source[(cur_height - 1) * width + i];
		unsigned char c = ((i <= byte_on_pixel) || (cur_height == 0)) ? 0 : source[(cur_height - 1) * width + i - byte_on_pixel];
		int p = a + b - c;
		int pa = abs(p - a);
		int pb = abs(p - b);
		int pc = abs(p - c);
		unsigned char ans;
		if (pa <= pb && pa <= pc)
			ans = (unsigned char)a;
		else if (pb <= pc)
			ans = (unsigned char)b;
		else
			ans = (unsigned char)c;
		source[cur_height * width + i] = (source[cur_height * width + i] + ans) % 256;
	}
}

int do_filter(unsigned char *decompress_source, long long int height, long long int width, int byte_on_pixel)
{
	for (int i = 0; i < height; i++)
	{
		int filter_metod = decompress_source[i * width];
		switch (filter_metod)
		{
		case 0:
			break;
		case 1:
			do_sub_filter(decompress_source, i, width, byte_on_pixel);
			break;
		case 2:
			if (i > 0)
				do_up_filter(decompress_source, i, width);
			break;
		case 3:
			do_average_filter(decompress_source, i, width, byte_on_pixel);
			break;
		case 4:
			do_paeth_filter(decompress_source, i, width, byte_on_pixel);
			break;
		default:
			fprintf(stderr, "%i is unsupported filter metod", filter_metod);
			return ERROR_DATA_INVALID;
		}
	}
	return SUCCESS;
}

int get_byte_on_pixel(int type)
{
	switch (type)
	{
	case 0:
		return 1;
	case 2:
		return 3;
	case 3:
		return 1;
	case 4:
		return 2;
	case 6:
		return 4;
	}
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc != 3 && argc != 4)
	{
		fprintf(stderr,
				"The parameter or number of parameters (argv) is incorrect\nfirst argument: input file\nsecond "
				"argument: output file\nthird - fifth arguments: background\n");
		return ERROR_PARAMETER_INVALID;
	}
	FILE *png = fopen(argv[1], "rb");
	if (png == NULL)
	{
		get_message(ERROR_CANNOT_OPEN_FILE);
		return ERROR_CANNOT_OPEN_FILE;
	}
	struct Ihdr ihdr;
	struct Idat idat;
	struct Plte plte;
	struct Bkgd bkgd;
	struct Trns trns;
	if (argc == 3)
		for (int i = 0; i < 3; i++)
			bkgd.background[i] = 0;
	idat.source = malloc(sizeof(unsigned char));
	plte.source = malloc(sizeof(unsigned char));
	bkgd.have_bkgd = false;
	trns.point_background = malloc(sizeof(unsigned char));
	trns.have_trns = false;
	if ((idat.source == NULL) || (plte.source == NULL) || (trns.point_background == NULL))
	{
		free(idat.source);
		free(plte.source);
		free(trns.point_background);
		get_message(ERROR_OUT_OF_MEMORY);
		return ERROR_OUT_OF_MEMORY;
	}
	int return_code = read_chunks(&ihdr, &idat, &plte, &bkgd, &trns, png);
	fclose(png);
	if (return_code != SUCCESS)
	{
		free(idat.source);
		free(plte.source);
		free(trns.point_background);
		get_message(return_code);
		return return_code;
	}
	return_code = get_background_color(argc, argv, &bkgd);
	if (return_code != SUCCESS)
	{
		free(idat.source);
		free(plte.source);
		free(trns.point_background);
		get_message(return_code);
		return return_code;
	}
	int byte_on_pixel = get_byte_on_pixel(ihdr.color_type);
	unsigned long decompress_len = ihdr.height * (ihdr.width + 1) * byte_on_pixel;
	unsigned char *decompress_source = malloc(sizeof(unsigned char) * decompress_len);
	if (decompress_source == NULL)
	{
		free(idat.source);
		free(plte.source);
		free(trns.point_background);
		get_message(ERROR_OUT_OF_MEMORY);
		return ERROR_OUT_OF_MEMORY;
	}
	return_code = do_decompress(decompress_source, &decompress_len, idat.source, idat.size);
	if (return_code != SUCCESS)
	{
		free(idat.source);
		free(plte.source);
		free(trns.point_background);
		free(decompress_source);
		get_message(return_code);
		return return_code;
	}
	return_code = do_filter(decompress_source, ihdr.height, ihdr.width * byte_on_pixel + 1, byte_on_pixel);
	if (return_code != SUCCESS)
	{
		free(idat.source);
		free(plte.source);
		free(trns.point_background);
		free(decompress_source);
		return return_code;
	}
	return_code = write_in_file(ihdr, plte, bkgd, trns, decompress_source, argv[2]);
	free(idat.source);
	free(plte.source);
	free(trns.point_background);
	free(decompress_source);
	if (return_code != SUCCESS)
	{
		get_message(return_code);
		return return_code;
	}
	return SUCCESS;
}