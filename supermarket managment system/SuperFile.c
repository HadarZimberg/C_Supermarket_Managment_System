#define  _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Address.h"
#include "General.h"
#include "fileHelper.h"
#include "SuperFile.h"
#include "myMacros.h"


int	saveSuperMarketToFile(const SuperMarket* pMarket, const char* fileName,
	const char* customersFileName)
{
	FILE* fp;
	fp = fopen(fileName, "wb");
	if (!fp) {
		printf("Error open supermarket file to write\n");
		return 0;
	}

	if (!writeStringToFile(pMarket->name, fp, "Error write supermarket name\n"))
	{
		fclose(fp);
		return 0;
	}

	if (!saveAddressToFile(&pMarket->location, fp))
	{
		fclose(fp);
		return 0;
	}
	int count = getNumOfProductsInList(pMarket);

	if (!writeIntToFile(count, fp, "Error write product count\n"))
	{
		fclose(fp);
		return 0;
	}

	Product* pTemp;
	NODE* pN = pMarket->productList.head.next; //first Node
	while (pN != NULL)
	{
		pTemp = (Product*)pN->key;
		if (!saveProductToFile(pTemp, fp))
		{
			fclose(fp);
			return 0;
		}
		pN = pN->next;
	}

	fclose(fp);

	saveCustomerToTextFile(pMarket->customerArr, pMarket->customerCount, customersFileName);

	return 1;
}

#define BIN
#ifdef BIN
int	loadSuperMarketFromFile(SuperMarket* pMarket, const char* fileName,
	const char* customersFileName)
{
	FILE* fp;
	fp = fopen(fileName, "rb");
	CHECK_MSG_RETURN_0(fp, "Error open company file\n");

	pMarket->name = readStringFromFile(fp, "Error reading supermarket name\n");
	if (!pMarket->name)
	{
		CLOSE_RETURN_0(fp);
	}

	
	if (!loadAddressFromFile(&pMarket->location, fp))
	{
		FREE_CLOSE_FILE_RETURN_0(fp, pMarket->name);
	}

	int count;
	if (!readIntFromFile(&count, fp, "Error reading product count\n"))
	{
		FREE_CLOSE_FILE_RETURN_0(fp, pMarket->name);
	}

	if (count > 0)
	{
		Product* pTemp;
		for (int i = 0; i < count; i++)
		{
			pTemp = (Product*)calloc(1, sizeof(Product));
			if (!pTemp)
			{
				printf("Allocation error\n");
				L_free(&pMarket->productList, freeProduct);
				free(pMarket->name);
				CLOSE_RETURN_0(fp);
			}
			if (!loadProductFromFile(pTemp, fp))
			{
				L_free(&pMarket->productList, freeProduct);
				FREE_CLOSE_FILE_RETURN_0(fp, pMarket->name);
			}
			if (!insertNewProductToList(&pMarket->productList, pTemp))
			{
				L_free(&pMarket->productList, freeProduct);
				FREE_CLOSE_FILE_RETURN_0(fp, pMarket->name);
			}
		}
	}

	fclose(fp);

	pMarket->customerArr = loadCustomerFromTextFile(customersFileName, &pMarket->customerCount);
	if (!pMarket->customerArr)
		return 0;

	return	1;

}

int saveSuperMarketToCompressedFile(const SuperMarket* pMarket, const char* fileName)
{
	FILE* fp;
	fp = fopen(fileName, "wb");
	if (!fp)
		return 0;

	BYTE data[2] = { 0 };
	int numOfProd = getNumOfProductsInList(pMarket);
	int marketNameLen = (int)strlen(pMarket->name);
	data[0] = (numOfProd) >> 2;
	data[1] = (numOfProd & 0x3) << 6 | marketNameLen;

	if (fwrite(&data, sizeof(BYTE), 2, fp) != 2)
	{
		fclose(fp);
		return 0;
	}

	if (fwrite(pMarket->name, sizeof(char), marketNameLen, fp) != marketNameLen)
	{
		fclose(fp);
		return 0;
	}

	BYTE count = (BYTE)pMarket->location.num;
	if (fwrite(&count, sizeof(BYTE), 1, fp) != 1)
	{
		fclose(fp);
		return 0;
	}

	if (!writeStringToFile(pMarket->location.street, fp, "error write street name\n"))
	{
		fclose(fp);
		return 0;
	}

	if (!writeStringToFile(pMarket->location.city, fp, "error write city name\n"))
	{
		fclose(fp);
		return 0;
	}

	Product* pTemp;
	NODE* pN = pMarket->productList.head.next; //first Node
	while (pN != NULL)
	{
		pTemp = (Product*)pN->key;
		if (!saveProductToCompFile(pTemp, fp))
		{
			fclose(fp);
			return 0;
		}
		pN = pN->next;
	}

	fclose(fp);
	return 1;
}

int loadSuperMarketFromCompressedFile(SuperMarket* pMarket, const char* fileName, const char* customersFileName)
{
	FILE* fp;
	fp = fopen(fileName, "rb");
	if (!fp)
		return 0;

	BYTE data[2];
	if (fread(&data, sizeof(BYTE), 2, fp) != 2)
	{
		fclose(fp);
		return 0;
	}

	//num of products:
	int numOfProducts = ((data[0] & 0xFF) << 2) | ((data[1] >> 6) & 0x03);

	//market name:
	int nameLen = data[1] & 0x3F;
	pMarket->name = (char*)calloc(nameLen + 1, sizeof(char));
	if (!pMarket->name)
	{
		fclose(fp);
		return 0;
	}
	if (fread(pMarket->name, sizeof(char), nameLen, fp) != nameLen)
	{
		free(pMarket->name);
		fclose(fp);
		return 0;
	}

	//Addess:

	//num-
	BYTE addess[1];
	if (fread(&addess, sizeof(BYTE), 1, fp) != 1)
	{
		free(pMarket->name);
		fclose(fp);
		return 0;
	}
	pMarket->location.num = addess[0];

	//street-
	pMarket->location.street = readStringFromFile(fp, "error read street name\n");
	if (!pMarket->location.street)
	{
		free(pMarket->name);
		free(pMarket->location.street);
		fclose(fp);
		return 0;
	}

	//city-
	pMarket->location.city = readStringFromFile(fp, "error read city name\n");
	if (!pMarket->location.city)
	{
		free(pMarket->name);
		free(pMarket->location.street);
		free(pMarket->location.city);
		fclose(fp);
		return 0;
	}

	//products:
	Product* pTemp;
	for (int i = 0; i < numOfProducts; i++)
	{
		pTemp = (Product*)calloc(1, sizeof(Product));
		if (!pTemp)
		{
			printf("Allocation error\n");
			L_free(&pMarket->productList, freeProduct);
			free(pMarket->name);
			CLOSE_RETURN_0(fp);
		}
		if (!loadProductFromCompressedFile(pTemp, fp))
		{
			L_free(&pMarket->productList, freeProduct);
			FREE_CLOSE_FILE_RETURN_0(fp, pMarket->name);
		}
		if (!insertNewProductToList(&pMarket->productList, pTemp))
		{
			L_free(&pMarket->productList, freeProduct);
			FREE_CLOSE_FILE_RETURN_0(fp, pMarket->name);
		}
	}

	fclose(fp);

	pMarket->customerArr = loadCustomerFromTextFile(customersFileName, &pMarket->customerCount);
	if (!pMarket->customerArr)
		return 0;

	return	1;
}




#else
int	loadSuperMarketFromFile(SuperMarket* pMarket, const char* fileName,
	const char* customersFileName)
{
	FILE* fp;
	fp = fopen(fileName, "rb");
	if (!fp)
	{
		printf("Error open company file\n");
		return 0;
	}

	//L_init(&pMarket->productList);


	pMarket->name = readStringFromFile(fp, "Error reading supermarket name\n");
	if (!pMarket->name)
	{
		fclose(fp);
		return 0;
	}

	if (!loadAddressFromFile(&pMarket->location, fp))
	{
		free(pMarket->name);
		fclose(fp);
		return 0;
	}

	fclose(fp);

	loadProductFromTextFile(pMarket, "Products.txt");


	pMarket->customerArr = loadCustomerFromTextFile(customersFileName, &pMarket->customerCount);
	if (!pMarket->customerArr)
		return 0;

	return	1;

}
#endif

int	loadProductFromTextFile(SuperMarket* pMarket, const char* fileName)
{
	FILE* fp;
	//L_init(&pMarket->productList);
	fp = fopen(fileName, "r");
	int count;
	fscanf(fp, "%d\n", &count);


	//Product p;
	Product* pTemp;
	for (int i = 0; i < count; i++)
	{
		pTemp = (Product*)calloc(1, sizeof(Product));
		myGets(pTemp->name, sizeof(pTemp->name), fp);
		myGets(pTemp->barcode, sizeof(pTemp->barcode), fp);
		fscanf(fp, "%d %f %d\n", &pTemp->type, &pTemp->price, &pTemp->count);
		insertNewProductToList(&pMarket->productList, pTemp);
	}

	fclose(fp);
	return 1;
}