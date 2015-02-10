/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Jan Solanti
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef LUPNG_USE_ZLIB
#include <miniz.h>
#else
#include <zlib.h>
#endif

#include "lupng.h"

#define PNG_NONE 0
#define PNG_IHDR 0x01
#define PNG_PLTE 0x02
#define PNG_IDAT 0x04
#define PNG_IEND 0x08

#define PNG_GRAYSCALE 0
#define PNG_TRUECOLOR 2
/* 24bpp RGB palette */
#define PNG_PALETTED 3
#define PNG_GRAYSCALE_ALPHA 4
#define PNG_TRUECOLOR_ALPHA 6

#define PNG_FILTER_NONE 0
#define PNG_FILTER_SUB 1
#define PNG_FILTER_UP 2
#define PNG_FILTER_AVERAGE 3
#define PNG_FILTER_PAETH 4

#define PNG_SIG_SIZE 8

#define PNG_DONE 1
#define PNG_OK 0
#define PNG_ERROR -1

#define BUF_SIZE 8192
#define MAX(x, y) (x > y ? x : y)



/********************************************************
 * CRC computation as per PNG spec
 ********************************************************/

/* Precomputed table of CRCs of all 8-bit messages
 using the polynomial from the PNG spec, 0xEDB88320L. */
static const uint32_t crcTable[] =
{
    0x0, 0x77073096, 0xEE0E612C, 0x990951BA, 0x76DC419, 0x706AF48F,
    0xE963A535, 0x9E6495A3, 0xEDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x9B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
    0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
    0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
    0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
    0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x1DB7106,
    0x98D220BC, 0xEFD5102A, 0x71B18589, 0x6B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0xF00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x86D3D2D,
    0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
    0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
    0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
    0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
    0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x3B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x4DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
    0xD6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0xA00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
    0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
    0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
    0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
    0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x26D930A, 0x9C0906A9, 0xEB0E363F,
    0x72076785, 0x5005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0xCB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0xBDBDF21, 0x86D3D2D4, 0xF1D4E242,
    0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
    0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
    0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
    0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

/* Update a running CRC with the bytes buf[0..len-1]--the CRC
 should be initialized to all 1's, and the transmitted value
 is the 1's complement of the final running CRC (see the
 crc() routine below)). */
static uint32_t updateCrc(uint32_t crc, unsigned char *buf,
                          int len)
{
    unsigned long c = crc;
    int n;

    for (n = 0; n < len; n++)
        c = crcTable[(c ^ buf[n]) & 0xFF] ^ (c >> 8);

    return c;
}

/* Return the CRC of the bytes buf[0..len-1]. */
static uint32_t crc(unsigned char *buf, int len)
{
    return updateCrc(0xFFFFFFFFL, buf, len) ^ 0xFFFFFFFFL;
}



/********************************************************
 * Helper structs
 ********************************************************/

typedef struct
{
    uint32_t length;
    uint8_t *type;
    uint8_t *data;
    uint32_t crc;
} PngChunk;

typedef struct {
    void *userPtr;
    PngReadProc readProc;
    PngWriteProc writeProc;
    int8_t chunksFound;

    /* IHDR info */
    uint32_t width;
    uint32_t height;
    uint8_t depth;
    uint8_t colorType;
    uint8_t channels;
    uint8_t compression;
    uint8_t filter;
    uint8_t interlace;

    /* PLTE info */
    uint32_t paletteItems;
    uint8_t *palette;

    /* fields used for (de)compression & (de-)filtering */
    z_stream stream;
    size_t scanlineBytes;
    int32_t currentCol;
    int32_t currentRow;
    uint32_t currentElem;
    size_t currentByte;
    int bytesPerPixel;
    uint8_t *currentScanline;
    uint8_t *previousScanline;
    uint8_t currentFilter;
    uint8_t interlacePass;
    size_t compressedBytes;

    /* used for constructing 16 bit deep pixels */
    int tmpCount;
    uint8_t tmpBytes[2];

    /* the output image */
    LuImage *img;
} PngInfoStruct;

/* PNG header: */
static const uint8_t PNG_SIG[] =
/*        P     N     G    \r    \n   SUB    \n   */
{0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};

static const int startingRow[] =  { 0, 0, 0, 4, 0, 2, 0, 1 };
static const int startingCol[] =  { 0, 0, 4, 0, 2, 0, 1, 0 };
static const int rowIncrement[] = { 1, 8, 8, 8, 4, 4, 2, 2 };
static const int colIncrement[] = { 1, 8, 8, 4, 4, 2, 2, 1 };



/********************************************************
 * Helper functions
 ********************************************************/

static inline void releaseChunk(PngChunk *chunk)
{
    /* Only release chunk->type since chunk->data points to the same memory. */
    free((void *)chunk->type);
    free((void *)chunk);
}

static inline uint32_t swap32(uint32_t n)
{
    union {
        unsigned char np[4];
        uint32_t i;
    } u;
    u.i = n;

    return ((uint32_t)u.np[0] << 24)  |
    ((uint32_t)u.np[1] << 16) |
    ((uint32_t)u.np[2] << 8)  |
    (uint32_t)u.np[3];
}

static inline uint16_t swap16(uint16_t n)
{
    union {
        unsigned char np[2];
        uint16_t i;
    } u;
    u.i = n;

    return ((uint16_t)u.np[0] << 8) | (uint16_t)u.np[1];
}

static int bytesEqual(const uint8_t *a, const uint8_t *b, size_t count)
{
    size_t i;
    for (i = 0; i < count; ++i)
    {
        if (*(a+i) != *(b+i))
            return 0;
    }

    return 1;
}



/********************************************************
 * Png filter functions
 ********************************************************/
static inline int absi(int val)
{
    return val > 0 ? val : -val;
}

static inline uint8_t raw(PngInfoStruct *info, int32_t col)
{
    if (col < 0)
        return 0;
    return info->currentScanline[col];
}

static inline uint8_t prior(PngInfoStruct *info, int32_t col)
{
    if (info->currentRow <= startingRow[info->interlacePass] || col < 0)
        return 0;
    return info->previousScanline[col];
}

static inline uint8_t paethPredictor(uint8_t a, uint8_t b, uint8_t c)
{
    unsigned int A = a, B = b, C = c;
    int p = (int)A + (int)B - (int)C;
    int pa = absi(p - (int)A);
    int pb = absi(p - (int)B);
    int pc = absi(p - (int)C);

    if (pa <= pb && pa <= pc)
        return a;
    if (pb <= pc)
        return b;
    return c;
}

static inline uint8_t deSub(PngInfoStruct *info, uint8_t filtered)
{
    return filtered + raw(info, info->currentByte-info->bytesPerPixel);
}

static inline uint8_t deUp(PngInfoStruct *info, uint8_t filtered)
{
    return filtered + prior(info, info->currentByte);
}

static inline uint8_t deAverage(PngInfoStruct *info, uint8_t filtered)
{
    uint16_t avg = (uint16_t)(raw(info, info->currentByte-info->bytesPerPixel)
                                + prior(info, info->currentByte));
    avg >>= 1;
    return filtered + avg;
}

static inline uint8_t dePaeth(PngInfoStruct *info, uint8_t filtered)
{
    return filtered + paethPredictor(
                        raw(info, info->currentByte-info->bytesPerPixel),
                        prior(info, info->currentByte),
                        prior(info, info->currentByte-info->bytesPerPixel));
}

static inline uint8_t none(PngInfoStruct *info)
{
    return raw(info, info->currentByte);
}

static inline uint8_t sub(PngInfoStruct *info)
{
    return raw(info, info->currentByte) - raw(info, info->currentByte-info->bytesPerPixel);
}

static inline uint8_t up(PngInfoStruct *info)
{
    return raw(info, info->currentByte) - prior(info, info->currentByte);
}

static inline uint8_t average(PngInfoStruct *info)
{
    uint16_t avg = (uint16_t)(raw(info, info->currentByte-info->bytesPerPixel)
                              + prior(info, info->currentByte));
    avg >>= 1;
    return raw(info, info->currentByte) - avg;
}

static inline uint8_t paeth(PngInfoStruct *info)
{
    return raw(info, info->currentByte) - paethPredictor(
                    raw(info, info->currentByte-info->bytesPerPixel),
                    prior(info, info->currentByte),
                    prior(info, info->currentByte-info->bytesPerPixel));
}



/********************************************************
 * Actual implementation
 ********************************************************/
static inline int parseIhdr(PngInfoStruct *info, PngChunk *chunk)
{
    if (info->chunksFound)
    {
        printf("PNG: malformed PNG file!\n");
        return PNG_ERROR;
    }

    info->chunksFound |= PNG_IHDR;
    info->width = swap32(*(uint32_t *)chunk->data);
    info->height = swap32(*((uint32_t *)chunk->data + 1));
    info->depth = *(chunk->data + 8);
    info->colorType = *(chunk->data + 9);
    info->compression = *(chunk->data + 10);
    info->filter = *(chunk->data + 11);
    info->interlace = *(chunk->data + 12);

    switch (info->colorType)
    {
        case PNG_GRAYSCALE:
            info->channels = 1;
            break;
        case PNG_TRUECOLOR:
            info->channels = 3;
            break;
        case PNG_PALETTED:
            info->channels = 3;
            break;
        case PNG_GRAYSCALE_ALPHA:
            info->channels = 2;
            break;
        case PNG_TRUECOLOR_ALPHA:
            info->channels = 4;
            break;
        default:
            printf("PNG: illegal color type: %i\n",
                   (unsigned int)info->colorType);
            return PNG_ERROR;
            break;
    }

    if ((info->colorType != PNG_GRAYSCALE && info->colorType != PNG_PALETTED &&
         info->depth < 8) ||
        (info->colorType == PNG_PALETTED && info->depth == 16) ||
        info->depth > 16)
    {
        printf("PNG: illegal bit depth for color type\n");
        return PNG_ERROR;
    }

    if (info->compression)
    {
        printf("PNG: unknown compression method: %i\n",
               (unsigned int)info->compression);
        return PNG_ERROR;
    }

    if (info->filter)
    {
        printf("PNG: unknown filter scheme: %i\n",
               (unsigned int)info->filter);
        return PNG_ERROR;
    }

    memset(&(info->stream), 0, sizeof(info->stream));
    if(inflateInit(&(info->stream)) != Z_OK)
    {
        printf("PNG: inflateInit failed!\n");
        return PNG_ERROR;
    }
    info->img = luImageCreate(info->width, info->height,
                              info->channels, info->depth < 16 ? 8 : 16);
    info->scanlineBytes = MAX((info->width * info->channels * info->depth) >> 3, 1);
    info->currentScanline = (uint8_t *)malloc(info->scanlineBytes);
    info->previousScanline = (uint8_t *)malloc(info->scanlineBytes);
    info->currentCol = -1;
    info->interlacePass = info->interlace ? 1 : 0;
    info->bytesPerPixel = MAX((info->channels * info->depth) >> 3, 1);
    if (!info->img || !info->currentScanline || !info->previousScanline)
    {
        printf("PNG: memory allocation failed!\n");
        return PNG_ERROR;
    }

    return PNG_OK;
}

static inline int parsePlte(PngInfoStruct *info, PngChunk *chunk)
{
    if (info->chunksFound & PNG_IDAT || !(info->chunksFound & PNG_IHDR))
    {
        printf("PNG: malformed PNG file!\n");
        return PNG_ERROR;
    }

    info->chunksFound |= PNG_PLTE;

    if (chunk->length % 3 != 0)
    {
        printf("PNG: invalid palette size!\n");
        return PNG_ERROR;
    }

    info->paletteItems = chunk->length/3;
    info->palette = (uint8_t *)malloc(chunk->length);
    if (!info->palette)
    {
        printf("PNG: memory allocation failed!\n");
        return PNG_ERROR;
    }
    memcpy(info->palette, chunk->data, chunk->length);

    return PNG_OK;
}

static inline void stretchBits(uint8_t inByte, uint8_t outBytes[8], int depth)
{
    int i;
    switch (depth) {
        case 1:
            for (i = 0; i < 8; ++i)
                outBytes[i] = (inByte >> (7-i)) & 0x01;
            break;

        case 2:
            outBytes[0] = (inByte >> 6) & 0x03;
            outBytes[1] = (inByte >> 4) & 0x03;
            outBytes[2] = (inByte >> 2) & 0x03;
            outBytes[3] = inByte & 0x03;
            break;

        case 4:
            outBytes[0] = (inByte >> 4) & 0x0F;
            outBytes[1] = inByte & 0x0F;
            break;

        default:
            break;
    }
}

/* returns: 1 if at end of scanline, 0 otherwise */
static inline int insertByte(PngInfoStruct *info, uint8_t byte)
{
    int advance = 0;
    const uint8_t scale[] = {0x00, 0xFF, 0x55, 0x00, 0x11, 0x00, 0x00, 0x00};

    /* for paletted images currentElem will always be 0 */
    size_t idx = info->currentRow * info->width * info->channels
                    + info->currentCol * info->channels
                    + info->currentElem;

    if (info->colorType != PNG_PALETTED)
    {
        if (info->depth == 8)
            info->img->data[idx] = byte;

        else if (info->depth < 8)
            info->img->data[idx] = byte * scale[info->depth];

        else /* depth == 16 */
        {
            info->tmpBytes[info->tmpCount] = byte;
            if (info->tmpCount) /* just inserted 2nd byte */
            {
                uint16_t val = *(uint16_t *)info->tmpBytes;
                val = swap16(val);
                info->tmpCount = 0;

                ((uint16_t *)(info->img->data))[idx] = val;
            }
            else
            {
                ++info->tmpCount;
                return 0;
            }
        }

        ++info->currentElem;
        if (info->currentElem >= info->channels)
        {
            advance = 1;
            info->currentElem = 0;
        }
    }
    else
    {
        /* The spec limits palette size to 256 entries */
        if (byte < info->paletteItems)
        {
            info->img->data[idx  ] = info->palette[3*byte  ];
            info->img->data[idx+1] = info->palette[3*byte+1];
            info->img->data[idx+2] = info->palette[3*byte+2];
        }
        else
        {
            printf("PNG: invalid palette index encountered!\n");
        }
        advance = 1;
    }

    if (advance)
    {
        /* advance to next pixel */
        info->currentCol += colIncrement[info->interlacePass];

        if (info->currentCol >= info->width)
        {
            uint8_t *tmp = info->currentScanline;
            info->currentScanline = info->previousScanline;
            info->previousScanline = tmp;

            info->currentCol = -1;
            info->currentByte = 0;

            info->currentRow += rowIncrement[info->interlacePass];
            if (info->currentRow >= info->height && info->interlace)
            {
                ++info->interlacePass;
                while (startingCol[info->interlacePass] >= info->width ||
                       startingRow[info->interlacePass] >= info->height)
                    ++info->interlacePass;
                info->currentRow = startingRow[info->interlacePass];
            }
            return 1;
        }
    }

    return 0;
}

static inline int parseIdat(PngInfoStruct *info, PngChunk *chunk)
{
    unsigned char filtered[BUF_SIZE];
    int status = Z_OK;

    if (!(info->chunksFound & PNG_IHDR))
    {
        printf("PNG: malformed PNG file!\n");
        return PNG_ERROR;
    }

    if (info->colorType == PNG_PALETTED && !(info->chunksFound & PNG_PLTE))
    {
        printf("PNG: palette required but missing!\n");
        return PNG_ERROR;
    }

    info->chunksFound |= PNG_IDAT;
    info->stream.next_in = (unsigned char *)chunk->data;
    info->stream.avail_in = chunk->length;
    do
    {
        info->stream.next_out = filtered;
        info->stream.avail_out = BUF_SIZE;
        status = inflate(&(info->stream), Z_NO_FLUSH);
        size_t decompressed = BUF_SIZE - info->stream.avail_out;
        size_t i;

        if (status != Z_OK &&
            status != Z_STREAM_END &&
            status != Z_BUF_ERROR &&
            status != Z_NEED_DICT)
        {
            printf("PNG: inflate error!\n");
            return PNG_ERROR;
        }

        for (i = 0; i < decompressed; ++i)
        {
            if (info->currentCol < 0)
            {
                info->currentCol = startingCol[info->interlacePass];
                info->currentFilter = filtered[i];
            }
            else
            {
                uint8_t rawByte = 0;
                uint8_t fullBytes[8] = {0};
                switch (info->currentFilter)
                {
                    case PNG_FILTER_NONE:
                        rawByte = filtered[i];
                        break;
                    case PNG_FILTER_SUB:
                        rawByte = deSub(info, filtered[i]);
                        break;
                    case PNG_FILTER_UP:
                        rawByte = deUp(info, filtered[i]);
                        break;
                    case PNG_FILTER_AVERAGE:
                        rawByte = deAverage(info, filtered[i]);
                        break;
                    case PNG_FILTER_PAETH:
                        rawByte = dePaeth(info, filtered[i]);
                        break;
                    default:
                        break;
                }

                info->currentScanline[info->currentByte] = rawByte;
                ++info->currentByte;

                if (info->depth < 8)
                {
                    int j;
                    stretchBits(rawByte, fullBytes, info->depth);
                    for (j = 0; j < 8/info->depth; ++j)
                        if(insertByte(info, fullBytes[j]))
                            break;
                }
                else
                    insertByte(info, rawByte);
            }
        }
    } while ((info->stream.avail_in > 0 || info->stream.avail_out == 0));

    return PNG_OK;
}

static inline PngChunk *readChunk(PngInfoStruct *info)
{
    PngChunk *chunk = (PngChunk *)malloc(sizeof(PngChunk));
    size_t read = 0;
    if (!chunk)
    {
        printf("PNG: memory allocation failed!\n");
        return 0;
    }

    info->readProc((void *)&chunk->length, 4, 1, info->userPtr);
    chunk->length = swap32(chunk->length);
    chunk->type = (uint8_t *)malloc(chunk->length + 4);
    chunk->data = chunk->type + 4;

    info->readProc((void *)chunk->type, 1, chunk->length + 4, info->userPtr);
    read = info->readProc((void *)&chunk->crc, 4, 1, info->userPtr);
    chunk->crc = swap32(chunk->crc);

    if (read != 1)
    {
        printf("PNG: read error\n");
        releaseChunk(chunk);
        return 0;
    }

    if (crc(chunk->type, chunk->length+4) != chunk->crc)
    {
        printf("PNG: CRC mismatch in \'%.4s\'\n", (char *)chunk->type);
        releaseChunk(chunk);
        return 0;
    }

    return chunk;
}

static inline int handleChunk(PngInfoStruct *info, PngChunk *chunk)
{
    /* critical chunk */
    if (!(chunk->type[0] & 0x20))
    {
        if (bytesEqual(chunk->type, (const uint8_t *)"IHDR", 4))
            return parseIhdr(info, chunk);
        if (bytesEqual(chunk->type, (const uint8_t *)"PLTE", 4))
            return parsePlte(info, chunk);
        if (bytesEqual(chunk->type, (const uint8_t *)"IDAT", 4))
            return parseIdat(info, chunk);
        if (bytesEqual(chunk->type, (const uint8_t *)"IEND", 4))
        {
            info->chunksFound |= PNG_IEND;
            if (!(info->chunksFound & PNG_IDAT))
            {
                printf("PNG: no IDAT chunk found\n");
                return PNG_ERROR;
            }
            return PNG_DONE;
        }
    }
    /* ignore ancillary chunks for now */

    return PNG_OK;
}

LuImage *luPngRead(PngReadProc readProc, void *userPtr)
{
    PngInfoStruct info =
    {
        userPtr,
        readProc,
        0,
        PNG_NONE
    };
    uint8_t signature[PNG_SIG_SIZE];
    int status = PNG_ERROR;

    info.readProc((void *)signature, 1, PNG_SIG_SIZE, userPtr);
    if (bytesEqual(signature, PNG_SIG, PNG_SIG_SIZE))
    {
        PngChunk *chunk;
        while ((chunk = readChunk(&info)))
        {
            status = handleChunk(&info, chunk);
            releaseChunk(chunk);

            if (status != PNG_OK)
                break;
        }
    }
    else
        printf("PNG: invalid header\n");

    if (info.currentScanline)
        free((void *)info.currentScanline);
    if (info.previousScanline)
        free((void *)info.previousScanline);
    if (info.palette)
        free((void *)info.palette);
    inflateEnd(&info.stream);

    if (status == PNG_DONE)
        return info.img;
    else
        if (info.img)
            luImageRelease(info.img);

    return 0;
}

static inline int writeIhdr(PngInfoStruct *info)
{
    static uint8_t buf[17];
    static const uint8_t colorType[] = {
            PNG_GRAYSCALE,
            PNG_GRAYSCALE_ALPHA,
            PNG_TRUECOLOR,
            PNG_TRUECOLOR_ALPHA
    };
    size_t written = 0;
    PngChunk c;

    if (info->img->channels > 4)
    {
        printf("PNG: too many channels in image\n");
        return PNG_ERROR;
    }

    c.length = swap32(13);
    c.type = buf; // 4 (type) + 4 + 4 + 5x1
    c.data = c.type + 4;

    memcpy((void *)c.type, (void *)"IHDR", 4);
    *(uint32_t *)(c.data)     = swap32((uint32_t)info->img->width);
    *(uint32_t *)(c.data + 4) = swap32((uint32_t)info->img->height);
    *(c.data + 8)  = info->img->depth;
    *(c.data + 9)  = colorType[info->img->channels-1];
    *(c.data + 10) = 0; // compression method
    *(c.data + 11) = 0; // filter method
    *(c.data + 12) = 0; // interlace method: none

    c.crc = swap32(crc(c.type, 17));

    written += info->writeProc((void *)&c.length, 4, 1, info->userPtr) * 4;
    written += info->writeProc((void *)c.type, 1, 4, info->userPtr);
    written += info->writeProc((void *)c.data, 1, 13, info->userPtr);
    written += info->writeProc((void *)&c.crc, 4, 1, info->userPtr) * 4;

    if (written != 25)
    {
        printf("PNG: write error\n");
        return PNG_ERROR;
    }

    return PNG_OK;
}

static inline int writeIdat(PngInfoStruct *info, uint8_t *buf, size_t buflen)
{
    size_t written = 0;
    PngChunk c;

    c.length = swap32(buflen-4);
    c.crc = swap32(crc(buf, buflen));

    written += info->writeProc((void *)&c.length, 4, 1, info->userPtr) * 4;
    written += info->writeProc((void *)buf, 1, buflen, info->userPtr);
    written += info->writeProc((void *)&c.crc, 4, 1, info->userPtr) * 4;

    if (written != buflen+8)
    {
        printf("PNG: write error\n");
        return PNG_ERROR;
    }
    
    return PNG_OK;
}

static inline void advanceBytep(PngInfoStruct *info, int is16bit)
{
    if (is16bit)
    {
        if (info->currentByte%2)
            --info->currentByte;
        else
            info->currentByte+=3;
    }
    else
        ++info->currentByte;
}

static inline size_t filterScanline(PngInfoStruct *info,
                                    uint8_t(*f)(PngInfoStruct *info),
                                    uint8_t filter,
                                    uint8_t *filterCandidate,
                                    int is16bit)
{
    size_t curSum = 0;
    filterCandidate[0] = filter;
    size_t fc;
    for (info->currentByte = is16bit ? 1 : 0, fc = 1;
        info->currentByte < info->scanlineBytes; ++fc, advanceBytep(info, is16bit) )
    {
        uint8_t val = f(info);
        filterCandidate[fc] = val;
        curSum += val;
    }

    return curSum;
}

/*
 * Processes the input image and calls writeIdat for every BUF_SIZE compressed
 * bytes.
 */
static inline int processPixels(PngInfoStruct *info)
{
    uint8_t idatBuf[BUF_SIZE+4] = {'I', 'D', 'A', 'T'};
    uint8_t *compressed = idatBuf+4;
    uint8_t *filterCandidate = (uint8_t *)malloc(info->scanlineBytes+1);
    uint8_t *bestCandidate = (uint8_t *)malloc(info->scanlineBytes+1);
    size_t minSum = (size_t)-1, curSum = 0;
    int status = Z_OK;
    int is16bit = info->img->depth == 16;

    if (!filterCandidate || !bestCandidate)
    {
        printf("PNG: memory allocation failed!\n");
    }

    memset(&(info->stream), 0, sizeof(info->stream));
    if(deflateInit(&(info->stream), Z_DEFAULT_COMPRESSION) != Z_OK)
    {
        printf("PNG: deflateInit failed!\n");
        free(filterCandidate);
        free(bestCandidate);
        return PNG_ERROR;
    }

    info->stream.avail_out = BUF_SIZE;
    info->stream.next_out = compressed;

    for (info->currentRow = 0; info->currentRow < info->img->height;
         ++info->currentRow)
    {
        int flush = (info->currentRow < info->img->height-1) ?
        Z_NO_FLUSH : Z_FINISH;
        minSum = (size_t)-1;

        /*
         * 1st time it doesn't matter, the filters never look at the previous
         * scanline when processing row 0. And next time it'll be valid.
         */
        info->previousScanline = info->currentScanline;
        info->currentScanline = info->img->data + (info->currentRow*info->scanlineBytes);

        /*
         * Try to choose the best filter for each scanline.
         * Breaks in case of overflow, but hey it's just a heuristic.
         */
        for (info->currentFilter = PNG_FILTER_NONE; info->currentFilter <= PNG_FILTER_PAETH; ++info->currentFilter)
        {

            switch (info->currentFilter)
            {
                case PNG_FILTER_NONE:
                    curSum = filterScanline(info, none, PNG_FILTER_NONE, filterCandidate, is16bit);
                    break;

                case PNG_FILTER_SUB:
                    curSum = filterScanline(info, sub, PNG_FILTER_SUB, filterCandidate, is16bit);
                    break;

                case PNG_FILTER_UP:
                    curSum = filterScanline(info, up, PNG_FILTER_UP, filterCandidate, is16bit);
                    break;

                case PNG_FILTER_AVERAGE:
                    curSum = filterScanline(info, average, PNG_FILTER_AVERAGE, filterCandidate, is16bit);
                    break;

                case PNG_FILTER_PAETH:
                    curSum = filterScanline(info, paeth, PNG_FILTER_PAETH, filterCandidate, is16bit);
                    break;

                default:
                    break;
            }

            if (curSum < minSum || !info->currentFilter)
            {
                uint8_t *tmp = bestCandidate;
                bestCandidate = filterCandidate;
                filterCandidate = tmp;
                minSum = curSum;
            }
        }

        info->stream.avail_in = info->scanlineBytes+1;
        info->stream.next_in = bestCandidate;

        // compress bestCandidate
        do
        {
            status = deflate(&info->stream, flush);

            if (info->stream.avail_out < BUF_SIZE)
            {
                writeIdat(info, idatBuf, BUF_SIZE-info->stream.avail_out+4);
                info->stream.next_out = compressed;
                info->stream.avail_out = BUF_SIZE;
            }
        } while ((flush == Z_FINISH && status != Z_STREAM_END)
                    || (flush == Z_NO_FLUSH && info->stream.avail_in));
        // TODO: fix loop conditions, apparently data gets truncated
    }

    return PNG_OK;
}

static inline int writeIend(PngInfoStruct *info)
{
    PngChunk c = { 0, (uint8_t *)"IEND", 0, 0 };
    size_t written = 0;
    c.crc = swap32(crc(c.type, 4));

    written += info->writeProc((void *)&c.length, 4, 1, info->userPtr) * 4;
    written += info->writeProc((void *)c.type, 1, 4, info->userPtr);
    written += info->writeProc((void *)&c.crc, 4, 1, info->userPtr) * 4;

    if (written != 12)
    {
        printf("PNG: write error\n");
        return PNG_ERROR;
    }

    return PNG_OK;
}

int luPngWrite(PngWriteProc writeProc, void *userPtr, LuImage *img)
{
    PngInfoStruct info = {
        userPtr,
        0,
        writeProc,
        PNG_NONE
    };

    info.img = img;
    info.bytesPerPixel = (info.img->channels * info.img->depth) >> 3;

    if (writeProc((void *)PNG_SIG, 1, PNG_SIG_SIZE, userPtr) != PNG_SIG_SIZE)
    {
        printf("PNG: write error\n");
        return PNG_ERROR;
    }

    if (writeIhdr(&info) != PNG_OK)
        return PNG_ERROR;

    info.scanlineBytes = (img->depth >> 3) * img->channels * img->width;
    if (processPixels(&info) != PNG_OK)
    {
        deflateEnd(&(info.stream));
        return PNG_ERROR;
    }

    deflateEnd(&(info.stream));
    return writeIend(&info);
}


void luImageRelease(LuImage *img)
{
    free((void *)img->data);
    free((void *)img);
}

LuImage *luImageCreate(size_t width, size_t height, uint8_t channels, uint8_t depth)
{
    LuImage *img;

    if (depth != 8 && depth != 16)
    {
        printf("Image: only bit depths 8 and 16 are supported!\n");
        return 0;
    }

    img = (LuImage *)malloc(sizeof(LuImage));
    if (!img)
        return 0;

    img->width = width;
    img->height = height;
    img->channels = channels;
    img->depth = depth;
    img->dataSize = (size_t)((depth >> 3) * width * height * channels);
    img->data = (uint8_t *)malloc(img->dataSize);

    return img;
}
