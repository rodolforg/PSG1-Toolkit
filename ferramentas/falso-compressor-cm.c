/*
 * Copyright © 2011 Rodolfo Ribeiro Gomes <rodolforg arr0ba gmail.com>
 *
 * Falso compressor CM
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

#define MSG_NOME_PROGRAMA "Falso compressor em arquivo CM\n"
#define MSG_COPYRIGHT "(C) 2011-05 RodolfoRG <rodolforg@gmail.com>\n"
#define MSG_USO "Uso: %s ArquivoUCM NovoArquivoCM\n"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>


uint8_t* carrega_arquivo(const char *nomeArquivo, size_t *tamanho);
int salva_arquivo(const char *nomeArquivo, const uint8_t* buffer, size_t tamanho);

int main(int argc, char*argv[])
{
	if (argc < 3)
	{
		fprintf(stderr, MSG_NOME_PROGRAMA);
		fprintf(stderr, MSG_COPYRIGHT);
		fprintf(stderr, MSG_USO, argv[0]);
		exit(EXIT_FAILURE);
	}
	const char *nomeArquivoUCM = argv[1];
	const char *nomeArquivoCM = argv[2];

	size_t tamanhoUCM = 0;
	// Carrega o arquivo descompactado
	uint8_t *bufferUCM = carrega_arquivo(nomeArquivoUCM, &tamanhoUCM);
	if (bufferUCM == NULL)
	{
		fprintf(stderr, "Ao carregar arquivo %s\n", nomeArquivoUCM);
		perror("Razão");
		exit(EXIT_FAILURE);
	}
	
	uint8_t *bufferCM = malloc(tamanhoUCM+2+4+4+tamanhoUCM/8); // "CM" TAMANHO_DESCOMPACTADO TAMANHO_COMPACTADO
	if (bufferCM == NULL)
	{
		perror("Alocando CM");
		free(bufferUCM);
		exit(EXIT_FAILURE);
	}
	
	bufferCM[0] = 'C';
	bufferCM[1] = 'M';
	// Tamanho descompactado
	bufferCM[2] = (tamanhoUCM) & 0x000000FF;
	bufferCM[3] = (tamanhoUCM >>  8) & 0x000000FF;
	bufferCM[4] = (tamanhoUCM >> 16) & 0x000000FF;
	bufferCM[5] = (tamanhoUCM >> 24) & 0x000000FF;
	// Tamanho compactado sem header
	bufferCM[6] = (tamanhoUCM) & 0x000000FF;
	bufferCM[7] = (tamanhoUCM >>  8) & 0x000000FF;
	bufferCM[8] = (tamanhoUCM >> 16) & 0x000000FF;
	bufferCM[9] = (tamanhoUCM >> 24) & 0x000000FF;
	// Copia o conteúdo
	memcpy(&bufferCM[10], bufferUCM, tamanhoUCM);
	// Descarta o conteúdo
	free(bufferUCM);
	// Preenche com controles inúteis de compactação: nenhuma
	memset(&bufferCM[10 + tamanhoUCM], 0, tamanhoUCM/8);

	
	// Salva
	if (!salva_arquivo(nomeArquivoCM, bufferCM, tamanhoUCM+2+4+4+tamanhoUCM/8))
	{
		fprintf(stderr, "Problemas ao salvar em %s\n", nomeArquivoCM);
		perror("Razão");
		free(bufferCM);
	}
	
	exit(EXIT_SUCCESS);
}

uint8_t* carrega_arquivo(const char *nomeArquivo, size_t *tamanho)
{
	FILE *arquivo;
	uint8_t * buffer;
	size_t _tamanho;

	if (nomeArquivo == NULL)
		return NULL;
	
	arquivo = fopen(nomeArquivo, "rb");
	if (arquivo == NULL)
	{
		if (tamanho != NULL)
			*tamanho = 0;
		return NULL;
	}
	
	fseek(arquivo, 0, SEEK_END);
	_tamanho = ftell(arquivo);
	fseek(arquivo, 0, SEEK_SET);
	
	buffer = malloc(_tamanho);
	if (buffer == NULL)
	{
		fclose(arquivo);
		if (tamanho != NULL)
			*tamanho = 0;
		return NULL;
	}
	
	if (fread (buffer, _tamanho, 1, arquivo) != 1)
	{
		free(buffer);
		fclose(arquivo);
		if (tamanho != NULL)
			*tamanho = 0;
		return NULL;
	}
	
	fclose(arquivo);
	if (tamanho != NULL)
		*tamanho = _tamanho;
	return buffer;
}

int salva_arquivo(const char *nomeArquivo, const uint8_t* buffer, size_t tamanho)
{
	FILE *arquivo;
	
	if (nomeArquivo == NULL || buffer == NULL)
		return 0;
	
	arquivo = fopen(nomeArquivo, "wb");
	if (arquivo == NULL)
	{
		return 0;
	}
	
	if (fwrite (buffer, tamanho, 1, arquivo) != 1)
	{
		fclose(arquivo);
		return 0;
	}
	
	fclose(arquivo);

	return 1;
}
