/*
 * Copyright © 2011,2012 Rodolfo Ribeiro Gomes <rodolforg arr0ba gmail.com>
 *
 * Extrator de arquivos de pacotes DAT
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

#define MSG_NOME_PROGRAMA "Extrator de arquivos DAT\n"
#define MSG_COPYRIGHT "(C) 2011-10 RodolfoRG <rodolforg@gmail.com>\n"
#define MSG_USO "Uso: %s [NomeArquivo_dat] [PastaArquivosSaída]\n"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define NOME_PADRAO_DAT "pacote.dat"

uint8_t* carrega_arquivo(const char *nomeArquivo, size_t *tamanho);
bool salva_arquivo(const char *nomeArquivo, const uint8_t *conteudo, size_t tamanho);
static int Little_Endian = -1;
static bool ehLittleEndian();
static uint32_t pega32LE(const uint8_t *dados);
uint8_t* extrai_arquivo_pacote_dat(const uint8_t *dat, const size_t tamanho_dat, const int id, size_t *tamanho_arquivo);
int pega_qtd_arquivos_pacote_dat(const uint8_t *dat, const size_t tamanho_dat);

int main(int argc, char*argv[])
{
	char * nome_pasta = ".";
	char * nome_arquivo_dat = NOME_PADRAO_DAT;
	int qtdArquivos;
	uint8_t *dat;
	size_t tamanho_dat;
	uint8_t *arquivo_extraido;
	size_t tamanho_extraido;
	
	Little_Endian = ehLittleEndian();
	
	if (argc >= 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "-?") == 0))
	{
		fprintf(stderr, MSG_NOME_PROGRAMA);
		fprintf(stderr, MSG_COPYRIGHT);
		fprintf(stderr, MSG_USO, argv[0]);
		exit(EXIT_SUCCESS);
	}
	
	if (argc >= 2)
		nome_arquivo_dat = argv[1];
	
	if (argc >= 3)
		nome_pasta = argv[2];
		
	dat = carrega_arquivo(nome_arquivo_dat, &tamanho_dat);
	if (dat == NULL)
	{
		perror(nome_arquivo_dat);
		fprintf(stderr, MSG_NOME_PROGRAMA);
		fprintf(stderr, MSG_COPYRIGHT);
		fprintf(stderr, MSG_USO, argv[0]);
		exit(EXIT_FAILURE);
	}

	qtdArquivos = pega_qtd_arquivos_pacote_dat(dat, tamanho_dat);
	if (qtdArquivos < 1)
	{
		free(dat);
		fprintf(stderr, "O arquivo %s não pôde ser corretamente interpretado! Ele é válido\n", nome_arquivo_dat);
		exit(EXIT_FAILURE);
	}
	
	// Pega cada arquivo e o salva
	int n;
	for (n = 0; n < qtdArquivos; n++)
	{
		arquivo_extraido = extrai_arquivo_pacote_dat(dat, tamanho_dat, n, &tamanho_extraido);
		if (arquivo_extraido == NULL)
		{
			fprintf(stderr, "Não foi possível extrair o arquivo %d do %s\n", n, nome_arquivo_dat);
			free(dat);
			exit(EXIT_FAILURE);
		}
		char nome_arquivo_extraido[2048];
		snprintf(nome_arquivo_extraido, 2048, "%s/arquivo%03i.cm", nome_pasta, n);
		if (!salva_arquivo(nome_arquivo_extraido, arquivo_extraido, tamanho_extraido))
		{
			fprintf(stderr, "Não foi possível salvar o arquivo extraído #%d do %s\n", n, nome_arquivo_dat);
			free(arquivo_extraido);
			free(dat);
			exit(EXIT_FAILURE);
		}
		free(arquivo_extraido);
	}
	
	free(dat);
	
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
		perror(nomeArquivo);
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

bool salva_arquivo(const char *nomeArquivo, const uint8_t* buffer, size_t tamanho)
{
	FILE *arquivo;
	
	if (nomeArquivo == NULL || buffer == NULL)
		return 0;
	
	arquivo = fopen(nomeArquivo, "wb");
	if (arquivo == NULL)
	{
		perror(nomeArquivo);
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

static bool ehLittleEndian()
{
	short teste_endian_a = 0, teste_endian_b = 0;
	char *ptr_teste_b = (char *)&teste_endian_b;
	teste_endian_a = 0x0102;
	ptr_teste_b[0] = 0x02;
	ptr_teste_b[1] = 0x01;

	return (teste_endian_a == teste_endian_b);
}

static uint32_t pega32LE(const uint8_t *dados)
{
	uint32_t numero;
	numero = *(uint32_t*)dados;
	if (!Little_Endian)
	{
		numero = ((numero & 0x0FF) << 24) | ((numero & 0x0FF00) << 8) | ((numero & 0x0FF0000) >> 8) || ((numero & 0xFF000000) >> 24);
	}
	
	return numero;
}

uint8_t* extrai_arquivo_pacote_dat(const uint8_t *dat, const size_t tamanho_dat, const int id, size_t *tamanho_arquivo)
{
	int qtdArquivos;
	off_t posicao_inicio;
	off_t posicao_fim;
	uint8_t *arquivo_extraido;
	
	if (tamanho_arquivo == NULL)
		return NULL;

	if (dat == NULL || tamanho_dat < 0x800 || id < 0)
	{
		*tamanho_arquivo = 0;
		return NULL;
	}
	
	qtdArquivos = pega32LE(&dat[0]);
	if (id >= qtdArquivos)
	{
		*tamanho_arquivo = 0;
		return NULL;
	}
	
	posicao_inicio = pega32LE(&dat[(id+1)*4]);
	posicao_fim = pega32LE(&dat[(id+2)*4]);
	
	*tamanho_arquivo = (posicao_fim - posicao_inicio)*0x800;
	
	if (posicao_inicio*0x800 + *tamanho_arquivo > tamanho_dat)
	{
		fprintf(stderr, "Arquivo mal formatado.\n");
		*tamanho_arquivo = 0;
		return NULL;
	}
	arquivo_extraido = malloc(*tamanho_arquivo);
	if (arquivo_extraido == NULL)
	{
		*tamanho_arquivo = 0;
		return NULL;
	}
	
	memcpy(arquivo_extraido, &dat[posicao_inicio*0x800], *tamanho_arquivo);
	
	return arquivo_extraido;
}

int pega_qtd_arquivos_pacote_dat(const uint8_t *dat, const size_t tamanho_dat)
{
	if (dat == NULL || tamanho_dat < 0x800)
	{
		return -1;
	}
	
	return pega32LE(&dat[0]);
}

