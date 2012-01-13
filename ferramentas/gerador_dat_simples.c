/*
 * Copyright © 2011 Rodolfo Ribeiro Gomes <rodolforg arr0ba gmail.com>
 *
 * Gerador de pacote DAT
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

#define MSG_NOME_PROGRAMA "Gerador de pacote DAT\n"
#define MSG_COPYRIGHT "(C) 2011-05 RodolfoRG <rodolforg@gmail.com>\n"
#define MSG_USO "Uso: %s PastaComArquivos_arquivoXXX.cm [NomeArquivoSaída]\n"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define NOME_PADRAO_ARQUIVO_DAT "pacote.dat"
#define PADRAO_NOME_ARQUIVO_A_EMPACOTAR "arquivo%03i.cm"

#define QTD_ARQUIVOS_A_EMPACOTAR 54

uint8_t* carrega_arquivo(const char *nomeArquivo, size_t *tamanho);
int gera_pacote_dat(const char *nome_arquivo_saida, uint8_t **arquivos, const size_t *tamanhos, int qtdArquivos);
void destroi_lista_ponteiros(void **lista, int qtd);
char ** gerar_lista_arquivos(const char *nome_pasta, int qtdArquivos);

int main(int argc, char*argv[])
{
	char * nome_pasta = NULL;
	char * nome_arquivo_saida = NOME_PADRAO_ARQUIVO_DAT;
	int qtdArquivos = QTD_ARQUIVOS_A_EMPACOTAR;
	char **nomesArquivos;
	uint8_t **arquivos;
	size_t *tamanhos;
	
	if (argc < 2)
	{
		fprintf(stderr, MSG_NOME_PROGRAMA);
		fprintf(stderr, MSG_COPYRIGHT);
		fprintf(stderr, MSG_USO, argv[0]);
		exit(EXIT_FAILURE);
	}
	
	nome_pasta = argv[1];
	if (argc > 2)
	{
		nome_arquivo_saida = argv[2];
	}

	nomesArquivos = gerar_lista_arquivos(nome_pasta, qtdArquivos);
	if (nomesArquivos == NULL)
		exit(EXIT_FAILURE);
	
	tamanhos = malloc(qtdArquivos*sizeof(size_t));
	if (tamanhos == NULL)
	{
		destroi_lista_ponteiros((void**)nomesArquivos, qtdArquivos);
		exit(EXIT_FAILURE);
	}
	
	arquivos = malloc(qtdArquivos*sizeof(uint8_t*));
	if (arquivos == NULL)
	{
		destroi_lista_ponteiros((void**)nomesArquivos, qtdArquivos);
		free(tamanhos);
		exit(EXIT_FAILURE);
	}
	
	// Carrega todos os arquivos para a memória
	int n;
	for (n = 0; n < qtdArquivos; n++)
	{
		arquivos[n] = carrega_arquivo(nomesArquivos[n], &tamanhos[n]);
		if (arquivos[n] == NULL)
		{
			fprintf(stderr, "ERRO: Não foi possível carregar o arquivo %s\n", nomesArquivos[n]);
			destroi_lista_ponteiros((void**)nomesArquivos, qtdArquivos);
			destroi_lista_ponteiros((void**)arquivos, qtdArquivos);
			free(tamanhos);
			exit(EXIT_FAILURE);
		}
	}
	
	// Escreve pacote.dat
	gera_pacote_dat(nome_arquivo_saida, arquivos, tamanhos, qtdArquivos);
	
	// Libera memória
	destroi_lista_ponteiros((void**)nomesArquivos, qtdArquivos);
	destroi_lista_ponteiros((void**)arquivos, qtdArquivos);
	free(tamanhos);
	
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



void destroi_lista_ponteiros(void **lista, int qtd)
{
	int n;
	for (n = 0; n < qtd; n++)
		free(lista[n]);
	free(lista);
}

int gera_pacote_dat(const char *nomeArquivo, uint8_t **arquivos, const size_t *tamanhos, int qtdArquivos)
{
	FILE *arquivo;
	
	if (nomeArquivo == NULL || arquivos == NULL || tamanhos == NULL)
		return 0;
	
	arquivo = fopen(nomeArquivo, "wb+");
	if (arquivo == NULL)
	{
		return 0;
	}
	
	uint32_t numero;
	numero = qtdArquivos;
	int offset = 0x800;
	fwrite(&numero, 4, 1, arquivo); // Little-endian
	numero = 1;
	int n;
	for (n = 0; n < qtdArquivos; n++)
	{
		// Escreve no cabeçalho
		fseek (arquivo, (n+1)*4, SEEK_SET);
		fwrite(&numero, 4, 1, arquivo); // Little-endian
		offset = numero*0x800;
		numero += (tamanhos[n]/0x800);
		if (tamanhos[n] % 0x800 > 0)
			numero++;
		// Escreve arquivo a empacotar
		fseek (arquivo, offset, SEEK_SET);
		fwrite(arquivos[n], tamanhos[n], 1, arquivo);		
	}
	
	// Coloca último limite
	fseek (arquivo, (n+1)*4, SEEK_SET);
	fwrite(&numero, 4, 1, arquivo); // Little-endian
	// TODO: Preenche para completar com zeros?
	offset = numero*0x800;
	numero = 0;
	fseek (arquivo, offset-1, SEEK_SET);
	fwrite(&numero, 1, 1, arquivo);
	
	fclose(arquivo);

	return 1;
}

char ** gerar_lista_arquivos(const char *nome_pasta, int qtdArquivos)
{
	char **lista;
	int n;
	
	if (nome_pasta == NULL)
		return NULL;

	lista = malloc(qtdArquivos*sizeof(char*));
	if (lista == NULL)
		return NULL;
	
	for (n = 0; n < qtdArquivos; n++)
	{
		char nome[2048];
		snprintf(nome, 2048, "%s/"PADRAO_NOME_ARQUIVO_A_EMPACOTAR, nome_pasta, n);
		lista[n] = strdup(nome);
	}

	return lista;
}

