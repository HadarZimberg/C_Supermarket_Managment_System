#define  _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include "Product.h"
#include "General.h"
#include "fileHelper.h"
#include "myMacros.h"


#define MIN_DIG 3
#define MAX_DIG 5

void initProduct(Product* pProduct)
{
	initProductNoBarcode(pProduct);
	getBorcdeCode(pProduct->barcode);
}

void initProductNoBarcode(Product* pProduct)
{
	initProductName(pProduct);
	pProduct->type = getProductType();
	pProduct->price = getPositiveFloat("Enter product price\t");
	pProduct->count = getPositiveInt("Enter product number of items\t");
}

void initProductName(Product* pProduct)
{
	do {
		printf("enter product name up to %d chars\n", NAME_LENGTH);
		myGets(pProduct->name, sizeof(pProduct->name), stdin);
	} while (checkEmptyString(pProduct->name));
}

void printProduct(const Product* pProduct)
{
	printf("%-20s %-10s\t", pProduct->name, pProduct->barcode);
	printf("%-20s %5.2f %10d\n", typeStr[pProduct->type], pProduct->price, pProduct->count);
}

int	saveProductToFile(const Product* pProduct, FILE* fp)
{
	if (fwrite(pProduct, sizeof(Product), 1, fp) != 1)
	{
		puts("Error saving product to file\n");
		return 0;
	}
	return 1;
}

int saveProductToCompFile(const Product* pProduct, FILE* fp)
{
	//compress the barcode:
	char barcode[BARCODE_LENGTH] = { '0' };
	for (int i = 0; i < BARCODE_LENGTH; i++)//save the barcode's characters as encrypted
	{
		if (pProduct->barcode[i] >= '0' && pProduct->barcode[i] <= '9') //is digit
			barcode[i] = pProduct->barcode[i] - '0';
		else if (pProduct->barcode[i] >= 'A' && pProduct->barcode[i] <= 'Z') //is letter
			barcode[i] = pProduct->barcode[i] - 'A' + 10;
	}

	//compress the name:
	char name[NAME_LENGTH_HW4 + 1];
	int nameLen = (int)strlen(pProduct->name);
	if (nameLen > 15)
		nameLen = 15;
	memcpy(name, pProduct->name, nameLen);
	name[nameLen] = '\0';

	//compress into the data array:
	BYTE data[6];
	data[0] = (barcode[0] << 2) | (barcode[1] >> 4);
	data[1] = ((barcode[1] & 0xF) << 4) | (barcode[2] >> 2);
	data[2] = ((barcode[2] & 0x3) << 6) | barcode[3];
	data[3] = (barcode[4] << 2) | (barcode[5] >> 4);
	data[4] = ((barcode[5] & 0xF) << 4) | (barcode[6] >> 2);
	data[5] = ((barcode[6] & 0x3) << 6) | (nameLen << 2) | pProduct->type;

	//write the compressed data to the file:
	if (fwrite(data, sizeof(BYTE), 6, fp) != 6)
		return 0;
	if (fwrite(name, sizeof(char), nameLen, fp) != nameLen)
		return 0;

	//compress the count and price:
	BYTE data2[3];
	int count = pProduct->count;
	int shekel = (int)(pProduct->price);
	int agora = (int)((pProduct->price - shekel) * 100);
	data2[0] = (count & 0xFF);
	data2[1] = ((agora & 0x7F) << 1) | (shekel >> 8);
	data2[2] = (shekel & 0xFF);

	//write the compressed data to the file:
	if (fwrite(data2, sizeof(BYTE), 3, fp) != 3)
		return 0;

	return 1;
}

int	loadProductFromFile(Product* pProduct, FILE* fp)
{
	if (fread(pProduct, sizeof(Product), 1, fp) != 1)
	{
		puts("Error reading product from file\n");
		return 0;
	}
	return 1;
}

int loadProductFromCompressedFile(Product* pProduct, FILE* fp)
{
	BYTE data[6];
	if (fread(&data, sizeof(BYTE), 6, fp) != 6)
		return 0;

	char barcode[BARCODE_LENGTH + 1] = { 0 };
	barcode[0] = (data[0] >> 2) + '0'; //add '0' to get the actual character
	barcode[1] = (((data[0] & 0x03) << 4) | (data[1] >> 4)) + '0';
	barcode[2] = (((data[1] & 0x0F) << 2) | (data[2] >> 6)) + '0';
	barcode[3] = (data[2] & 0x3F) + '0';
	barcode[4] = (data[3] >> 2) + '0';
	barcode[5] = (((data[3] & 0x03) << 4) | (data[4] >> 4)) + '0';
	barcode[6] = (((data[4] & 0x0F) << 2) | (data[5] >> 6)) + '0';
	for (int i = 0; i < 7; i++)
	{
		if (barcode[i] > '9') //if ABC value-
			barcode[i] += 7; //convert to the real character by ascii value
	}
	barcode[7] = '\0';

	char name[16] = { 0 };
	int nameLen = (data[5] >> 2) & 0xF;
	if (fread(name, sizeof(char), nameLen, fp) != nameLen)
		return 0;

	int type = data[5] & 0x3;

	BYTE data2[3];
	if (fread(&data2, sizeof(BYTE), 3, fp) != 3)
		return 0;

	int count = data2[0];

	int agora = (data2[0] >> 1) & 0x7F;
	int shekel = ((data2[1] & 0x1) << 8) | ((data2[2]) & 0xFF);
	float price;
	if (agora > 9)
		price = (shekel + ((float)(agora % 100) / 100));
	else
		price = (shekel + ((float)(agora % 10) / 10));
	

	strcpy(pProduct->barcode, barcode);
	strcpy(pProduct->name, name);
	pProduct->count = count;
	pProduct->price = price;
	pProduct->type = type;

	return 1;
}

void getBorcdeCode(char* code)
{
	char temp[MAX_STR_LEN];
	char msg[MAX_STR_LEN];
	sprintf(msg, "Code should be of %d length exactly\n"
		"UPPER CASE letter and digits\n"
		"Must have %d to %d digits\n"
		"First and last chars must be UPPER CASE letter\n"
		"For example A12B40C\n",
		BARCODE_LENGTH, MIN_DIG, MAX_DIG);
	int ok = 1;
	int digCount = 0;
	do {
		ok = 1;
		digCount = 0;
		printf("Enter product barcode ");
		getsStrFixSize(temp, MAX_STR_LEN, msg);
		if (strlen(temp) != BARCODE_LENGTH)
		{
			puts(msg);
			ok = 0;
		}
		else {
			//check and first upper letters
			if (!isupper(temp[0]) || !isupper(temp[BARCODE_LENGTH - 1]))
			{
				puts("First and last must be upper case letters\n");
				ok = 0;
			}
			else {
				for (int i = 1; i < BARCODE_LENGTH - 1; i++)
				{
					if (!isupper(temp[i]) && !isdigit(temp[i]))
					{
						puts("Only upper letters and digits\n");
						ok = 0;
						break;
					}
					if (isdigit(temp[i]))
						digCount++;
				}
				if (digCount < MIN_DIG || digCount > MAX_DIG)
				{
					puts("Incorrect number of digits\n");
					ok = 0;
				}
			}
		}

	} while (!ok);

	strcpy(code, temp);
}


eProductType getProductType()
{
	int option;
	printf("\n\n");
	do {
		printf("Please enter one of the following types\n");
		for (int i = 0; i < eNofProductType; i++)
			printf("%d for %s\n", i, typeStr[i]);
		scanf("%d", &option);
	} while (option < 0 || option >= eNofProductType);
	getchar();
	return (eProductType)option;
}

const char* getProductTypeStr(eProductType type)
{
	if (type < 0 || type >= eNofProductType)
		return NULL;
	return typeStr[type];
}

int	isProduct(const Product* pProduct, const char* barcode)
{
	if (strcmp(pProduct->barcode, barcode) == 0)
		return 1;
	return 0;
}

int	compareProductByBarcode(const void* var1, const void* var2)
{
	const Product* pProd1 = (const Product*)var1;
	const Product* pProd2 = (const Product*)var2;

	return strcmp(pProd1->barcode, pProd2->barcode);
}


void updateProductCount(Product* pProduct)
{
	int count;
	do {
		printf("How many items to add to stock?");
		scanf("%d", &count);
	} while (count < 1);
	pProduct->count += count;
}


void freeProduct(Product* pProduct)
{
	//nothing to free!!!!
}