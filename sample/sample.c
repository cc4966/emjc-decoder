#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <sbix_emjc_decode.h>
#include <png.h>

uint32_t extract32(const uint8_t* const p) {
    return p[3] + (p[2] << 8) + (p[1] << 16) + (p[0] << 24);
}

uint16_t extract16(const uint8_t* const p) {
    return p[1] + (p[0] << 8);
}

void writeToPng(const char* const filepath, png_bytep bitmap, int width, int height) {
    FILE *fp = fopen(filepath, "wb");
    png_structp writeStruct = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop infoStruct = png_create_info_struct(writeStruct);
    png_init_io(writeStruct, fp);
    png_set_IHDR(writeStruct, infoStruct, width, height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_bytepp lines = (png_bytepp)malloc(height * sizeof(png_bytep));
    for (int i = 0; i < height; ++i) {
        lines[i] = &bitmap[i * width * 4];
        for (int j = 0; j < width; ++j) {
            // BGRA to RGBA
            png_byte temp = bitmap[(i * width + j) * 4];
            bitmap[(i * width + j) * 4] = bitmap[(i * width + j) * 4 + 2];
            bitmap[(i * width + j) * 4 + 2] = temp;
        }
    }
    png_write_info(writeStruct, infoStruct);
    png_write_image(writeStruct, lines);
    png_write_end(writeStruct, infoStruct);
    png_destroy_write_struct(&writeStruct, &infoStruct);
    fclose(fp);
    free(lines);
}

void extractGlyph(const uint8_t* const header, uint32_t length, uint32_t fontIndex, uint32_t strikeIndex, uint32_t glyphIndex) {
    int16_t originOffsetX = (int16_t)extract16(&header[0]);
    int16_t originOffsetY = (int16_t)extract16(&header[2]);
    if (strncmp((const char* const)&header[4], "emjc", 4) == 0) {
        const uint8_t* const p = &header[8];
        size_t data_length = length - 8;

        uint16_t width = emjc_width(p, data_length);
        uint16_t height = emjc_width(p, data_length);
        size_t bufferSize = emjc_decode_buffer_size(p, data_length);
        uint8_t *buffer = (uint8_t *)malloc(bufferSize);
        assert(emjc_decode_buffer(buffer, p, length, NULL) == 0);
        char filename[256];
        snprintf(filename, 256, "%u-%u-gid%u (%ux%u|%d,%d).png", fontIndex, strikeIndex, glyphIndex, width, height, originOffsetX, originOffsetY);
        writeToPng(filename, buffer, width, height);
        free(buffer);
    }
}

void extractStrike(const uint8_t* const p, uint32_t fontIndex, uint32_t strikeIndex, uint16_t numGlyphs) {
    const uint8_t* const header = p;
    uint16_t ppem = extract16(&header[0]);
    uint16_t ppi = extract16(&header[2]);
    for (uint32_t glyphIndex = 0; glyphIndex < numGlyphs; ++glyphIndex) {
        uint32_t glyphDataOffset = extract32(&p[4 + 4 * glyphIndex]);
        uint32_t nextGlyphDataOffset = extract32(&p[4 + 4 * glyphIndex + 4]);
        if (glyphDataOffset < nextGlyphDataOffset) {
            extractGlyph(&p[glyphDataOffset], nextGlyphDataOffset - glyphDataOffset, fontIndex, strikeIndex, glyphIndex);
        }
    }
}

void extractSbix(const uint8_t* const p, uint32_t fontIndex, uint16_t numGlyphs) {
    const uint8_t* const header = p;
    uint16_t version = extract16(&header[0]);
    assert(version == 1);
    uint16_t flags = extract16(&header[2]);
    assert(flags == 0 || flags == 1);
    uint32_t numStrikes = extract32(&header[4]);
    for (uint32_t strikeIndex = 0; strikeIndex < numStrikes; ++strikeIndex) {
        uint32_t strikeOffset = extract32(&p[8 + 4 * strikeIndex]);
        extractStrike(&p[strikeOffset], fontIndex, strikeIndex, numGlyphs);
    }
}

void extractEmjcGlyphs(FILE* fp, uint32_t fontIndex, uint16_t numGlyphs) {
    char header[12];
    assert(fread(header, 1, 12, fp) == 12);
    uint16_t numTables = extract16((const uint8_t* const)&header[4]);
    for (uint16_t tableIndex = 0; tableIndex < numTables; ++tableIndex) {
        char tableRecord[16];
        assert(fread(tableRecord, 1, 16, fp) == 16);
        if (strncmp(tableRecord, "sbix", 4) == 0) {
            uint32_t offset = extract32((const uint8_t* const)&tableRecord[8]);
            uint32_t length = extract32((const uint8_t* const)&tableRecord[12]);
            long cur = ftell(fp);
            assert(fseek(fp, offset, SEEK_SET) == 0);
            uint8_t *p = (uint8_t *)malloc(length);
            assert(fread((char *)p, 1, length, fp) == length);
            extractSbix(p, fontIndex, numGlyphs);
            assert(fseek(fp, cur, SEEK_SET) == 0);
            free(p);
            return;
        }
    }
}

uint16_t extractNumGlyphs(FILE* fp) {
    char header[12];
    assert(fread(header, 1, 12, fp) == 12);
    uint16_t numTables = extract16((const uint8_t* const)&header[4]);
    for (uint16_t tableIndex = 0; tableIndex < numTables; ++tableIndex) {
        char tableRecord[16];
        assert(fread(tableRecord, 1, 16, fp) == 16);
        if (strncmp(tableRecord, "maxp", 4) == 0) {
            uint32_t offset = extract32((const uint8_t* const)&tableRecord[8]);
            uint32_t length = extract32((const uint8_t* const)&tableRecord[12]);
            assert(fseek(fp, offset, SEEK_SET) == 0);
            uint8_t *p = (uint8_t *)malloc(length);
            assert(fread((char *)p, 1, length, fp) == length);
            uint16_t numGlyphs = extract16(&p[4]);
            free(p);
            return numGlyphs;
        }
    }
    abort();
}

void extractFont(FILE* fp, uint32_t tableDirectoryOffset, uint32_t fontIndex) {
    assert(fseek(fp, tableDirectoryOffset, SEEK_SET) == 0);
    uint16_t numGlyphs = extractNumGlyphs(fp);
    assert(fseek(fp, tableDirectoryOffset, SEEK_SET) == 0);
    extractEmjcGlyphs(fp, fontIndex, numGlyphs);
}

void extractEmjcGlyphsForEachFont(FILE* fp, const char* const header) {
    uint16_t majorVersion = extract16((const uint8_t* const)&header[4]);
    assert(majorVersion == 1 || majorVersion == 2);
    uint16_t minorVersion = extract16((const uint8_t* const)&header[6]);
    assert(minorVersion == 0);
    uint32_t numFonts = extract32((const uint8_t* const)&header[8]);
    for (uint32_t fontIndex = 0; fontIndex < numFonts; ++fontIndex) {
        uint32_t tableDirectoryOffset;
        assert(fread((char *)&tableDirectoryOffset, 1, 4, fp) == 4);
        long cur = ftell(fp);
        tableDirectoryOffset = extract32((const uint8_t* const)&tableDirectoryOffset);
        extractFont(fp, tableDirectoryOffset, fontIndex);
        assert(fseek(fp, cur, SEEK_SET) == 0);
    }
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("input emoji font file using `emjc` format\n");
        exit(-1);
    }
    FILE* fp = fopen(argv[1], "rb");
    if (fp == NULL) {
        printf("could not open file\n");
        exit(-1);
    }
    char header[12];
    assert(fread(header, 1, 12, fp) == 12);
    if (strncmp(header, "ttcf", 4) == 0) {
        extractEmjcGlyphsForEachFont(fp, header);
    } else {
        extractFont(fp, 0, 0);
    }
    fclose(fp);
    return 0;
}
