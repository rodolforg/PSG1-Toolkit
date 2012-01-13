/*
 * Copyright © 2011 Rodolfo Ribeiro Gomes <rodolforg arr0ba gmail.com>
 *
 * Insersor de texto em arquivo UCM
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

#define MSG_NOME_PROGRAMA "Insersor de textos para [PS2] Phantasy Star generations: 1\n"
#define MSG_COPYRIGHT "(C) 2011-07 RodolfoRG <rodolforg@gmail.com>\n"
#define MSG_USO "Uso: %s ArquivoUCM ArquivoMensagem NovoArquivoUCM\n"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>


/*
 * O arquivo texto tem formato 
 * ﻿--- PONTEIRO_EM_HEXADECIMAL ---
 * Texto
 * ﻿--- PONTEIRO_EM_HEXADECIMAL ---
 * Texto
 */

typedef struct Mensagem
{
	uint32_t endereco_original;
	uint32_t endereco_novo;
	char * texto;
} Mensagem;

uint8_t* carrega_arquivo(const char *nomeArquivo, size_t *tamanho);
int salva_arquivo(const char *nomeArquivo, const uint8_t* buffer, size_t tamanho);
int carrega_mensagens(const char *nomeArquivo, Mensagem **mensagens);
void fecha_mensagens(Mensagem *mensagens, int qtdMensagens);
int anexa_mensagens_ao_UCM(Mensagem *mensagens, int qtd, uint8_t** buffer, size_t *tamanho);
off_t busca_uint32_le(const uint8_t* buffer, size_t tamanho, uint32_t numero, off_t pos_inicial);

static char *marca_fim_de_mensagem(char *mensagem);

int main(int argc, char*argv[])
{
	if (argc < 4)
	{
		fprintf(stderr, MSG_NOME_PROGRAMA);
		fprintf(stderr, MSG_COPYRIGHT);
		fprintf(stderr, MSG_USO, argv[0]);
		exit(EXIT_FAILURE);
	}
	const char *nomeArquivoUCM = argv[1];
	const char *nomeArquivoMsg = argv[2];
	const char *nomeArquivoUCMNovo = argv[3];

	size_t tamanhoUCM = 0;
	int n;
	uint8_t *bufferUCM = carrega_arquivo(nomeArquivoUCM, &tamanhoUCM);
	Mensagem *mensagens = NULL;
	int qtdMensagens = carrega_mensagens(nomeArquivoMsg, &mensagens);
	anexa_mensagens_ao_UCM(mensagens, qtdMensagens, &bufferUCM, &tamanhoUCM);
	for (n = 0; n < qtdMensagens; n++)
	{
		int qtdOcorrencias = 0;
		off_t pos = 0;
		while ((pos = busca_uint32_le(bufferUCM, tamanhoUCM, mensagens[n].endereco_original, pos)) != (off_t)-1)
		{
			qtdOcorrencias++;
			bufferUCM[pos+0] = (mensagens[n].endereco_novo >>  0) & 0x000000FF;
			bufferUCM[pos+1] = (mensagens[n].endereco_novo >>  8) & 0x000000FF;
			bufferUCM[pos+2] = (mensagens[n].endereco_novo >> 16) & 0x000000FF;
			bufferUCM[pos+3] = (mensagens[n].endereco_novo >> 24) & 0x000000FF;
			pos+=4;
		}
		if (qtdOcorrencias == 0)
			fprintf(stdout, "NORMAL: A mensagem #%i (posição 0x%08X) não afetou ponteiro algum.\n", n, mensagens[n].endereco_original);
		else if (qtdOcorrencias == 1)
			fprintf(stdout, "NORMAL: A mensagem #%i (posição 0x%08X) afetou um ponteiro.\n", n, mensagens[n].endereco_original);
		else
			fprintf(stderr, "*ATENÇÃO*: A mensagem #%i (posição 0x%08X) afetou %i ponteiros diferentes.\n", n, mensagens[n].endereco_original, qtdOcorrencias);
	}
	fecha_mensagens(mensagens, qtdMensagens);
	salva_arquivo(nomeArquivoUCMNovo, bufferUCM, tamanhoUCM);
	
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

int carrega_mensagens(const char *nomeArquivo, Mensagem **mensagens)
{
	FILE *arquivo;
	int qtdMensagens;

	if (nomeArquivo == NULL)
		return 0;
	
	arquivo = fopen(nomeArquivo, "r");
	if (arquivo == NULL)
	{
		return 0;
	}

	qtdMensagens = 0;
	*mensagens = NULL;
	char linha[LINE_MAX];
	uint32_t endereco;
	int leu_mensagem = 0;
	int leu_endereco = 0;
	
	// Verifica se há Byte Order Mark
	if (fread(linha, 1, 3, arquivo) < 3)
	{
		fprintf(stderr, "ERRO: tentando verificar a presença de Marcador de ordenação de bytes\n");
		perror("Razão");
		return 0;
	}
	if (memcmp(linha, "\xEF\xBB\xBF", 3) != 0)
	{
		// Não há BOM, voltar ao começo.
		fseek(arquivo, 0 , SEEK_SET);
	}
	// Busca linhas
	while (!feof(arquivo))
	{
		if (fgets(linha, LINE_MAX, arquivo) == NULL)
		{
			if (!feof(arquivo))
			{
				fprintf(stderr, "ERRO: obtendo linha de texto do arquivo (possível mensagem #%i)\n", qtdMensagens);
				perror("Razão");
			}
			break;
		}
		
		int comprimento_linha = strlen(linha);

		// Remove a quebra de linha do fgets
		if (linha[comprimento_linha -1] == 0x0A)
		{
			linha[comprimento_linha -1] = 0x00;
			comprimento_linha--;
		}
		if (linha[comprimento_linha -1] == 0x0D)
		{
			linha[comprimento_linha -1] = 0x00;
			comprimento_linha--;
		}

		
		// Tenta ler marcador de endereço
		if (sscanf(linha, "--- %X", &endereco) < 1)
		{
			if (!leu_endereco)
			{
				fprintf(stderr, "ERRO: lendo endereço da mensagem #%i ('%s')\n", qtdMensagens, linha);
				perror("Razão");
				break;
			}
			
			// É mensagem
			int comprimento_mensagem = 0;
			char *mensagem = (*mensagens)[qtdMensagens-1].texto;
			if (mensagens != NULL && mensagem != NULL)
				comprimento_mensagem = strlen(mensagem);
		
			char *tmp = realloc(mensagem, comprimento_mensagem + comprimento_linha + 2);
			if (tmp == NULL)
			{
				perror("Adicionando mensagem");
				break;
			}
			mensagem = tmp;
			// Anexa quebra de linha (@) se a mensagem anterior não continha
			if (comprimento_mensagem >= 1 && mensagem[comprimento_mensagem-1] != '@')
			{
				mensagem[comprimento_mensagem] = '@';
				comprimento_mensagem++;
			}
			if (comprimento_mensagem >= 2 && mensagem[comprimento_mensagem-1] == '@' && mensagem[comprimento_mensagem-2] & 0x80)
			{
				mensagem[comprimento_mensagem] = '@';
				comprimento_mensagem++;
			}
			// Acrescenta a nova linha à mensagem
			memcpy(mensagem + comprimento_mensagem, linha, comprimento_linha);
			mensagem[comprimento_mensagem+comprimento_linha] = 0;
			(*mensagens)[qtdMensagens-1].texto = mensagem;

			leu_mensagem = 1;
		}
		else
		{
			if (!leu_mensagem && leu_endereco)
			{
				fprintf(stderr, "ERRO: lendo endereço da mensagem #%i ('%s')\nA mensagem anterior não continha textos.\n", qtdMensagens, linha);
				break;
			}
			if (leu_mensagem)
			{
				// Finaliza mensagem anterior
				(*mensagens)[qtdMensagens-1].texto = marca_fim_de_mensagem((*mensagens)[qtdMensagens-1].texto);
			}
			
			leu_endereco = 1;
			leu_mensagem = 0;

			// Reserva espaço para novas mensagens
			Mensagem *temp = realloc(*mensagens, (qtdMensagens+1)*sizeof(Mensagem));
			if (temp == NULL)
			{
				perror("Fechando mensagem");
				break;
			}
			*mensagens = temp;
			(*mensagens)[qtdMensagens].endereco_original = endereco;
			(*mensagens)[qtdMensagens].texto = NULL;
			(*mensagens)[qtdMensagens].endereco_novo = 0;
			
			qtdMensagens ++;
			continue;
		}

	}

	if (leu_mensagem)
	{
		// Finaliza mensagem anterior
		(*mensagens)[qtdMensagens-1].texto = marca_fim_de_mensagem((*mensagens)[qtdMensagens-1].texto);
	}

	fclose(arquivo);
	return qtdMensagens;
}

static char *marca_fim_de_mensagem(char *mensagem)
{
	char *msg;
	
	int comprimento_mensagem = strlen(mensagem);
	
	if (comprimento_mensagem >= 1 && mensagem[comprimento_mensagem -1] == '@')
	{
		mensagem[comprimento_mensagem -1] = 0x00;
		comprimento_mensagem--;
	}

	if (comprimento_mensagem >= 2 && mensagem[comprimento_mensagem-2] == 0x25 && mensagem[comprimento_mensagem-1] == 0x5C)
		msg = mensagem; // Nada a se fazer
	else
	{
		// Injeta os marcadores de fim de mensagem
		msg = realloc(mensagem, comprimento_mensagem + 2 + 1); // Mensagem + marcador de fim %¥ + byte 0
		msg[comprimento_mensagem+0] = 0x25;
		msg[comprimento_mensagem+1] = 0x5C;
		msg[comprimento_mensagem+2] = 0x00;
	}
	
	return msg;
}

void fecha_mensagens(Mensagem *mensagens, int qtdMensagens)
{
	int n;
	
	if (mensagens == NULL)
		return;
	
	for (n = 0; n < qtdMensagens; n++)
	{
		free(mensagens[n].texto);
	}
	
	free(mensagens);
}

int anexa_mensagens_ao_UCM(Mensagem *mensagens, int qtd, uint8_t** buffer, size_t *tamanho)
{
	if (mensagens == NULL || buffer == NULL || *buffer == NULL)
		return 0;

	int n;
	uint8_t *tmp;
	int length;
	
	for (n = 0; n < qtd; n++)
	{
		mensagens[n].endereco_novo = (*tamanho);
		length = strlen(mensagens[n].texto);
		tmp = realloc(*buffer, (*tamanho) + length );
		if (tmp == NULL)
		{
			fprintf(stderr, "Anexando mensagens ao arquivo (#%i)\n", n);
			perror("Razão");
			return 0;
		}
		*buffer = tmp;
		memcpy(&(*buffer)[*tamanho], mensagens[n].texto, length);
		*tamanho = (*tamanho) + length;
	}
	
	return 1;
}

off_t busca_uint32_le(const uint8_t* buffer, size_t tamanho, uint32_t numero, off_t pos_inicial)
{
	if (buffer == NULL)
		return -1;
	
	uint8_t n8[4];
	n8[0] = numero  & 0x0ff;
	numero >>= 8;
	n8[1] = numero  & 0x0ff;
	numero >>= 8;
	n8[2] = numero  & 0x0ff;
	numero >>= 8;
	n8[3] = numero  & 0x0ff;
	
	while (pos_inicial + 4 <= tamanho)
	{
		if (buffer[pos_inicial+0] == n8[0] && buffer[pos_inicial+1] == n8[1] && buffer[pos_inicial+2] == n8[2] && buffer[pos_inicial+3] == n8[3])
			return pos_inicial;
		pos_inicial ++;
	}
	
	return (off_t)-1;
}

