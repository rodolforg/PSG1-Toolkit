/*
 * Copyright © 2010,2012 Rodolfo Ribeiro Gomes <rodolforg arr0ba gmail.com>
 *
 * Extrator de mensagens de arquivos UCM
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#define NOME_PADRAO_ARQUIVO_SAIDA "textos.sjis"

#define MSG_NOME_PROGRAMA "Extrator de textos para arquivos UCM\n"
#define MSG_COPYRIGHT "(C) 2010-09 RodolfoRG <rodolforg@gmail.com>\n"
#define MSG_USO "Uso: %s NOME_ARQUIVO [NOME_ARQUIVO_SAIDA]\n"

void exibe_modo_uso(const char *aplicativo)
{
	fprintf(stderr, MSG_NOME_PROGRAMA);
	fprintf(stderr, MSG_COPYRIGHT);
	fprintf(stderr, MSG_USO, aplicativo);
}


int main(int argc, char*argv[])
{
	char * nome_arquivo = NULL;
	char * nome_arquivo_saida = NOME_PADRAO_ARQUIVO_SAIDA;

	char assinatura[2];
	size_t tamanho_arquivo = 0;
	unsigned char * buffer_arquivo = NULL;

	setlocale(LC_ALL, "");

	if (argc < 2)
	{
		exibe_modo_uso(argv[0]);
		exit(1);
	}


	nome_arquivo = argv[1];

	if (argc > 2)
	{
		nome_arquivo_saida = argv[2];
	}


	FILE * arq = fopen(nome_arquivo, "rb");

	if (arq == NULL)
	{
		fprintf(stderr, "Não foi possível abrir o arquivo %s\n", nome_arquivo);
		perror(nome_arquivo);
		exit(1);
	}

	// Confere se é um arquivo aparentemente válido, isto é, não é um CM
	int bytes_lidos = fread(&assinatura, sizeof(assinatura), 1, arq);
	if (bytes_lidos < 1)
	{
		fprintf(stderr, "Não conseguiu ler do arquivo\n");
		if (ferror (arq))
		{
			perror(nome_arquivo);
		}
		else if (feof (arq))
		{
			fprintf(stderr, "Atingiu o fim do arquivo\n");
		}
		fclose(arq);
		exit(1);
	}

	if (assinatura[0]=='C' && assinatura[1]=='M')
	{
		fprintf(stderr, "Não é um arquivo descomprimido válido: contém a assinatura CM\n");
		fclose(arq);
		exit(1);
	}


	// Descobre o tamanho do arquivo
	if (fseek(arq, 0, SEEK_END) != 0)
	{
		fprintf(stderr, "Não foi possível deslocar até o fim do arquivo.");
		perror(nome_arquivo);
		fclose(arq);
		exit(1);
	}
	tamanho_arquivo = ftell(arq);
	if (fseek(arq, 0, SEEK_SET) != 0)
	{
		fprintf(stderr, "Não foi possível deslocar até o começo do arquivo.");
		perror(nome_arquivo);
		fclose(arq);
		exit(1);
	}

	// Carrega o arquivo para a memória
	buffer_arquivo = malloc(tamanho_arquivo);
	if (buffer_arquivo == NULL)
	{
		perror("Carregando arquivo");
		fclose(arq);
		exit(1);
	}
	bytes_lidos = fread(buffer_arquivo, 1, tamanho_arquivo, arq);
	if (bytes_lidos < tamanho_arquivo)
	{
		fprintf(stderr, "Não conseguiu ler do arquivo\n");
		if (ferror (arq))
		{
			perror(nome_arquivo);
		}
		else if (feof (arq))
		{
			fprintf(stderr, "Atingiu o fim do arquivo\n");
		}
		free(buffer_arquivo);
		fclose(arq);
		exit(1);
	}
	fclose(arq);

	// Abre o arquivo de saída
	arq = fopen(nome_arquivo_saida, "w");

	if (arq == NULL)
	{
		fprintf(stderr, "Não foi possível abrir o arquivo %s\n", nome_arquivo_saida);
		perror(nome_arquivo_saida);
		free(buffer_arquivo);
		exit(1);
	}


	off_t ptr_atual = 0;
	off_t ptr_anterior = -1;
	off_t ptr_demarcador = -1;
	// Busca pelo próximo separador de mensagens: 0x25 0x5C ou 0x25 0x2A
	int achou_separador_msg;
	int achou_separador_msg_sim_nao;
	while (1)
	{
		achou_separador_msg = 0;
		achou_separador_msg_sim_nao = 0;
		while ( ptr_atual + 1 < tamanho_arquivo)
		{
			if (buffer_arquivo[ptr_atual] == 0x25 && buffer_arquivo[ptr_atual+1] == 0x5C)
			{
				achou_separador_msg = 1;
				break;
			}
			if (buffer_arquivo[ptr_atual] == 0x25 && buffer_arquivo[ptr_atual+1] == 0x2A)
			{
				achou_separador_msg_sim_nao = 1;
				break;
			}
			// FIXME: Não é só 0x32 0x00 0x00 0x00 ...  0x00 0x00 0x00 0x00 também apareceu!
			if (buffer_arquivo[ptr_atual] == 0x32 && buffer_arquivo[ptr_atual+1] == 0x00)
			{
				if (ptr_atual + 3 < tamanho_arquivo)
				{
					if (buffer_arquivo[ptr_atual+2] == 0x00 && buffer_arquivo[ptr_atual+3] == 0x00)
					{
						ptr_demarcador = ptr_atual;
					}
				}
			}
			else if (buffer_arquivo[ptr_atual] == 0x00 && buffer_arquivo[ptr_atual+1] == 0x00)
			{
				if (ptr_atual + 3 < tamanho_arquivo)
				{
					if (buffer_arquivo[ptr_atual+2] == 0x00 && buffer_arquivo[ptr_atual+3] == 0x00)
					{
						ptr_demarcador = ptr_atual;
					}
				}
			}
			ptr_atual++;
		}
		if ( ptr_atual + 1 >= tamanho_arquivo)
		{
			// Fim da busca
			break;
		}
		// Entre o ponto anterior e o atual... há o finalizador 0x32 0x00 0x00 0x00 ?
		// FIXME: Caso inicial? -1 e -1
		if (ptr_demarcador == -1 && ptr_anterior == -1)
		{
			ptr_anterior = 0;
		}

		if (ptr_anterior < ptr_demarcador)
		{
			// Fim do bloco de texto é no ptr_anterior
			// Atualizando ptr_anterior para o começo de um novo bloco de texto
			ptr_anterior = ptr_demarcador+4;
		}
		// TODO: Garantir que é só texto SHIFT-JIS por meio do Iconv

		// Grava o texto:
		if (!achou_separador_msg_sim_nao)
			buffer_arquivo[ptr_atual] = 0;
		else
		{
			achou_separador_msg_sim_nao = buffer_arquivo[ptr_atual+2];
			buffer_arquivo[ptr_atual+2] = 0;
		}
		fprintf(arq, "--- %08X ---\n", ptr_anterior);
		fprintf(arq, "%s\n", buffer_arquivo + ptr_anterior);

		if (achou_separador_msg_sim_nao)
		{
			buffer_arquivo[ptr_atual+2] = achou_separador_msg_sim_nao;
		}
		ptr_anterior = ptr_atual + 2;
		ptr_atual ++;
	}

	fclose(arq);
	free(buffer_arquivo);
	return 0;
}

