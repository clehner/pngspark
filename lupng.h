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

#ifdef __cplusplus
extern "C" {
#endif

#pragma once
#include <stdint.h>

typedef struct {
    int32_t width;
    int32_t height;
    uint8_t channels;
    uint8_t depth; // must be 8 or 16
    size_t dataSize;
    uint8_t *data;
} LuImage;

typedef size_t (*PngReadProc)(void *outPtr, size_t size, size_t count, void *userPtr);
typedef size_t (*PngWriteProc)(const void *inPtr, size_t size, size_t count, void *userPtr);

/**
 * Creates a new Image object with the specified attributes.
 * The data store of the Image is allocated but its contents are undefined.
 * Only 8 and 16 bits deep images with 1-4 channels are supported.
 */
LuImage *luImageCreate(size_t width, size_t height, uint8_t channels, uint8_t depth);

/**
 * Releases the memory associated with the given Image object.
 */
void luImageRelease(LuImage *img);

/**
 * Decodes a PNG image with the provided read function into a LuImage struct
 *
 * @param readProc a function pointer to a user-defined function to use for
 * reading the PNG data.
 * @param userPtr an opaque pointer provided as an argument to readProc
 */
LuImage *luPngRead(PngReadProc readProc, void *userPtr);

/**
 * Encodes a LuImage struct to PNG and writes it out using a user-defined write
 * function.
 *
 * @param writeProc a function pointer to a user-defined function that will be
 * used for writing the final PNG data.
 * @param userPtr an opaque pointer provided as an argument to writeProc
 * @param img the LuImage to encode
 */
int luPngWrite(PngWriteProc writeProc, void *userPtr, LuImage *img);

#ifdef __cplusplus
}
#endif
