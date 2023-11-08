#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

void RLECompressFile(char* inputFilename, char* outputFilename) {
    FILE *inputFile = fopen(inputFilename, "r");
    FILE *outputFile = fopen(outputFilename, "w");
    
    if (!inputFile || !outputFile) {
        perror("Erro ao abrir arquivo");
        exit(1);
    }
    
    char currentChar, previousChar = fgetc(inputFile);
    int count = 1;
    
    while ((currentChar = fgetc(inputFile)) != EOF) {
        if (currentChar == previousChar) {
            count++;
        } else {
            fprintf(outputFile, "%d%c", count, previousChar);
            previousChar = currentChar;
            count = 1;
        }
    }
    
    fprintf(outputFile, "%d%c", count, previousChar);
    
    fclose(inputFile);
    fclose(outputFile);
}

void RLEDecompressFile(char* inputFilename, char* outputFilename) {
    FILE *inputFile = fopen(inputFilename, "r");
    FILE *outputFile = fopen(outputFilename, "w");
    
    if (!inputFile || !outputFile) {
        perror("Erro ao abrir arquivo");
        exit(1);
    }
    
    int count;
    char currentChar;
    
    while (fscanf(inputFile, "%d%c", &count, &currentChar) == 2) {
        for (int i = 0; i < count; i++) {
            fputc(currentChar, outputFile);
        }
    }
    
    fclose(inputFile);
    fclose(outputFile);
}

void calculateCompressionRateAndFactor(bool isRLE, char* originalFilename, char* compressedFilename) {
    const char* fileReadMethod = isRLE ? "r" : "rb";

    printf("%s", fileReadMethod);
    
    FILE *originalFile = fopen(originalFilename, fileReadMethod);
    FILE *compressedFile = fopen(compressedFilename, fileReadMethod);

    if (!originalFile || !compressedFile) {
        perror("Erro ao abrir arquivo");
        exit(1);
    }
    
    int fileCalcMethod = isRLE ? 0L : 0;
    
    fseek(originalFile, fileCalcMethod, SEEK_END);
    fseek(compressedFile, fileCalcMethod, SEEK_END);
    
    long originalSize = ftell(originalFile);
    long compressedSize = ftell(compressedFile);
    
    fclose(originalFile);
    fclose(compressedFile);
    
    if (originalSize == 0) {
        printf("O arquivo original está vazio, a taxa e o fator de compressão não podem ser calculados.\n");
        return;
    }
    
    double compressionRate = (double)compressedSize / originalSize;
    double compressionFactor = isRLE ? 1.0 / compressionRate : (double)originalSize / compressedSize;
    
    printf("Arquivo original: %s (tamanho: %ld bytes)\n", originalFilename, originalSize);
    printf("Arquivo comprimido: %s (tamanho: %ld bytes)\n", compressedFilename, compressedSize);
    printf("Taxa de compressão: %.2f\n", compressionRate);
    printf("Fator de compressão: %.2f\n", compressionFactor);
}

#define MAX_TABLE_SIZE 4096 

typedef struct {
    int code;
    char* string;
} LZWCode;

LZWCode* initLZWDictionary() {
    LZWCode* dictionary = (LZWCode*)malloc(MAX_TABLE_SIZE * sizeof(LZWCode));
    for (int i = 0; i < 256; i++) {
        dictionary[i].string = (char*)malloc(2 * sizeof(char));
        dictionary[i].string[0] = i;
        dictionary[i].string[1] = '\0';
        dictionary[i].code = i;
    }
    return dictionary;
}

int findIndex(LZWCode* dictionary, int size, const char* string) {
    for (int i = 0; i < size; i++) {
        if (strcmp(dictionary[i].string, string) == 0) {
            return i;
        }
    }
    return -1;
}

void addStringToDictionary(LZWCode* dictionary, int* size, const char* string) {
    dictionary[*size].string = strdup(string);
    dictionary[*size].code = *size;
    (*size)++;
}

void LZWCompressFile(char* inputFilename, char* outputFilename) {
    FILE *inputFile = fopen(inputFilename, "rb");
    FILE *outputFile = fopen(outputFilename, "wb");

    if (!inputFile || !outputFile) {
        perror("Erro ao abrir arquivo");
        exit(1);
    }

    LZWCode* dictionary = initLZWDictionary();
    int dictionarySize = 256;
    
    char currentChar;
    char* currentString = (char*)malloc(MAX_TABLE_SIZE * sizeof(char));
    currentString[0] = '\0';
    char* newString = NULL;
    
    while (fread(&currentChar, sizeof(char), 1, inputFile)) {
        newString = (char*)malloc(strlen(currentString) + 2);
        sprintf(newString, "%s%c", currentString, currentChar);
        
        if (findIndex(dictionary, dictionarySize, newString) != -1) {
            strcpy(currentString, newString);
        } else {
            int code = findIndex(dictionary, dictionarySize, currentString);
            fwrite(&dictionary[code].code, sizeof(int), 1, outputFile);
            if (dictionarySize < MAX_TABLE_SIZE) {
                addStringToDictionary(dictionary, &dictionarySize, newString);
            }
            strcpy(currentString, "");
            currentString[0] = currentChar;
            currentString[1] = '\0';
        }
        free(newString);
    }
    
    if (strlen(currentString) != 0) {
        int code = findIndex(dictionary, dictionarySize, currentString);
        fwrite(&dictionary[code].code, sizeof(int), 1, outputFile);
    }
    
    for (int i = 0; i < dictionarySize; i++) {
        free(dictionary[i].string);
    }
    free(dictionary);
    free(currentString);
    
    fclose(inputFile);
    fclose(outputFile);
}

void LZWDecompressFile(char* inputFilename, char* outputFilename) {
    FILE *inputFile = fopen(inputFilename, "rb");
    FILE *outputFile = fopen(outputFilename, "wb");

    if (!inputFile || !outputFile) {
        perror("Erro ao abrir arquivo");
        exit(1);
    }

    LZWCode* dictionary = initLZWDictionary();
    int dictionarySize = 256;

    int currentCode, previousCode;
    char* string;
    char* newString;
    char character;

    fread(&previousCode, sizeof(int), 1, inputFile);
    fwrite(dictionary[previousCode].string, sizeof(char), strlen(dictionary[previousCode].string), outputFile);

    while (fread(&currentCode, sizeof(int), 1, inputFile)) {
        if (currentCode >= dictionarySize) {
            string = strdup(dictionary[previousCode].string);
            newString = (char*)malloc(strlen(string) + 2);
            sprintf(newString, "%s%c", string, string[0]);
        } else {
            newString = strdup(dictionary[currentCode].string);
        }

        fwrite(newString, sizeof(char), strlen(newString), outputFile);

        character = newString[0];
        string = (char*)malloc(strlen(dictionary[previousCode].string) + 2);
        sprintf(string, "%s%c", dictionary[previousCode].string, character);
        if (dictionarySize < MAX_TABLE_SIZE) {
            addStringToDictionary(dictionary, &dictionarySize, string);
        }
        previousCode = currentCode;

        free(string);
        free(newString);
    }

    for (int i = 0; i < dictionarySize; i++) {
        free(dictionary[i].string);
    }
    free(dictionary);
    
    fclose(inputFile);
    fclose(outputFile);
}

int main() {
    char* RLEoriginalFile1 = "microdados2021_arq1.txt";
    char* LZWoriginalFile1 = "microdados2021_arq1.lzw";
    char* RLEcompressedFile1 = "microdados2021_arq1_compressed.txt";
    char* RLEdecompressedFile1 = "microdados2021_arq1_decompressed.txt";
    char* LZWcompressedFile1 = "microdados2021_arq1_compressed.lzw";
    char* LZWdecompressedFile1 = "microdados2021_arq1_decompressed.lzw";
    
    char* RLEoriginalFile3 = "microdados2021_arq3.txt";
    char* LZWoriginalFile3 = "microdados2021_arq3.lzw";
    char* RLEcompressedFile3 = "microdados2021_arq3_compressed.txt";
    char* RLEdecompressedFile3 = "microdados2021_arq3_decompressed.txt";
    char* LZWcompressedFile3 = "microdados2021_arq3_compressed.lzw";
    char* LZWdecompressedFile3 = "microdados2021_arq3_decompressed.lzw";

    // RLE Compressão, Descompressão e Calculo de Compressão para arq1
    
    // RLECompressFile(RLEoriginalFile1, RLEcompressedFile1);
    // calculateCompressionRateAndFactor(true, RLEoriginalFile1, RLEcompressedFile1);
    // RLEDecompressFile(RLEcompressedFile1, RLEdecompressedFile1);

    // RLE Compressão, Descompressão e Calculo de Compressão para arq3

    // RLECompressFile(RLEoriginalFile3, RLEcompressedFile3);
    // calculateCompressionRateAndFactor(true, RLEoriginalFile3, RLEcompressedFile3);
    // RLEDecompressFile(RLEcompressedFile3, RLEdecompressedFile3);

    // LZW Compressão, Descompressão e Calculo de Compressão para arq1

    // LZWCompressFile(LZWoriginalFile1, LZWcompressedFile1);
    // LZWDecompressFile(LZWcompressedFile1, LZWdecompressedFile1);
    // calculateCompressionRateAndFactor(false, LZWoriginalFile1, LZWcompressedFile1);

    // LZW Compressão, Descompressão e Calculo de Compressão para arq3

    // LZWCompressFile(LZWoriginalFile3, LZWcompressedFile3);
    // LZWDecompressFile(LZWcompressedFile3, LZWdecompressedFile3);
    // calculateCompressionRateAndFactor(false, LZWoriginalFile3, LZWcompressedFile3);

    return 0;
}