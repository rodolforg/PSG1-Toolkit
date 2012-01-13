/*
 * Copyright © 2009,2012 Rodolfo Ribeiro Gomes <rodolforg arr0ba gmail.com>
 *
 * Descompressor de arquivo CM
 *   A pedido de OrakioRob (Roberto) <orakio arr0ba gazetadealgol.com.br>
 *
 * Lógica de descompressão elaborada por Ignitz <ignitzhjfk arr0ba hotmail.com>
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

#define NOME_PADRAO_ARQUIVO_DESCOMPRIMIDO "descomprimido.ucm"

#define MSG_NOME_PROGRAMA "Descompressor de arquivos CM\n"
#define MSG_COPYRIGHT "(C) 2009-03 RodolfoRG <rodolforg@gmail.com>\n"
#define MSG_USO "Uso: %s NOME_ARQUIVO [OFFSET] [NOME_ARQUIVO_SAIDA]\n"

#define TAMANHO_MAXIMO_BUFFER ((unsigned long int) 15*1024*1024)

int main(int argc, char*argv[])
{
	char * nome_arquivo = NULL;
	char * nome_arquivo_saida = NOME_PADRAO_ARQUIVO_DESCOMPRIMIDO;
	unsigned long offset_arquivo = 0;
	
	char assinatura[2];
	long tamanho_descomprimido = 0; // 4 bytes
	unsigned char * buffer_comprimido = NULL;
	long tamanho_comprimido = 0;	// 4 bytes
	unsigned char * buffer_descomprimido = NULL;
	unsigned char * buffer_flags = NULL;


	if (argc < 2)
	{
		fprintf(stderr, MSG_NOME_PROGRAMA);
		fprintf(stderr, MSG_COPYRIGHT);
		fprintf(stderr, MSG_USO, argv[0]);
		exit(1);
	}


	short teste_endian_a = 0, teste_endian_b = 0;
	char *ptr_teste_b = (char *)&teste_endian_b;
	teste_endian_a = 0x0102;
	ptr_teste_b[0] = 0x02;
	ptr_teste_b[1] = 0x01;

	if (teste_endian_a != teste_endian_b)
	{
		// FIXME: Deveria não existir isso aqui... Mas a preguiça comandou :)
		fprintf(stderr, MSG_NOME_PROGRAMA);
		fprintf(stderr, MSG_COPYRIGHT);
		fprintf(stderr, "Programa não funciona neste sistema: processador não é little-endian\n");
		exit(1);
	}

	nome_arquivo = argv[1];
	
	if (argc > 2)
	{
		if (sscanf(argv[2], "%lX", &offset_arquivo) < 1)
		{
			fprintf(stderr, MSG_NOME_PROGRAMA);
			fprintf(stderr, MSG_COPYRIGHT);
			fprintf(stderr, "O parâmetro offset não é um número hexadecimal válido\n");
			exit(1);
		}
		
		if (argc > 3)
		{
			nome_arquivo_saida = argv[3];
		}

	}
	
	
	FILE * arq = fopen(nome_arquivo, "rb");
	
	if (arq == NULL)
	{
		fprintf(stderr, "Não foi possível abrir o arquivo %s\n", nome_arquivo);
		perror(nome_arquivo);
		exit(1);
	}
	
	if (fseek(arq, offset_arquivo, SEEK_SET) != 0)
	{
		fprintf(stderr, "Não foi possível deslocar até a posição %08lX\n", offset_arquivo);
		perror(nome_arquivo);
		fclose(arq);
		exit(1);
	}

	// Confere se é um arquivo CM válido
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
	
	if (assinatura[0]!='C' || assinatura[1]!='M')
	{
		fprintf(stderr, "%s: Não é um arquivo comprimido válido: Erro na assinatura CM\n", nome_arquivo);
		fclose(arq);
		exit(1);
	}

		
	// Confere a sanidade do tamanho comprimido x tamanho descomprimido
	bytes_lidos = fread(&tamanho_descomprimido, 4, 1, arq);
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
	// FIXME: Se big endian, inverter o tamanho_descomprimido (ele é armazenado em L-E no arquivo)

	bytes_lidos = fread(&tamanho_comprimido, 4, 1, arq);
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
	// FIXME: Se big endian, inverter o tamanho_comprimido (ele é armazenado em L-E no arquivo)

	if (tamanho_comprimido > tamanho_descomprimido)
	{
		fprintf(stderr, "Não é um arquivo comprimido válido: tamanho descomprimido menor que comprimido\n");
		fclose(arq);
		exit(1);
	}

	if (tamanho_descomprimido > TAMANHO_MAXIMO_BUFFER)
	{
		fprintf(stderr, "O arquivo descomprimido é maior que %08lX: %08lX\n", TAMANHO_MAXIMO_BUFFER, tamanho_descomprimido);
		fclose(arq);
		exit(1);
	}
	if (tamanho_comprimido > TAMANHO_MAXIMO_BUFFER)
	{
		fprintf(stderr, "O arquivo comprimido é maior que %08lX: %08lX\n", TAMANHO_MAXIMO_BUFFER, tamanho_comprimido);
		fclose(arq);
		exit(1);
	}
	
	buffer_comprimido = (unsigned char *) malloc(tamanho_comprimido);
	buffer_descomprimido = (unsigned char *) malloc(tamanho_descomprimido);
	buffer_flags = (unsigned char *) malloc(tamanho_comprimido / 8 + 1);
	//unsigned char buffer_comprimido[tamanho_comprimido];
	//unsigned char buffer_descomprimido[tamanho_descomprimido];
	//unsigned char buffer_flags[tamanho_comprimido / 8 + 1];
	
	if (buffer_comprimido == NULL || buffer_descomprimido == NULL || buffer_flags == NULL)
	{
		fprintf(stderr, "Não conseguiu alocar memória\n");
		fclose(arq);
		free(buffer_comprimido);
		free(buffer_descomprimido);
		free(buffer_flags);
		exit(1);
	}
	
	printf("Arquivo CM:\n\tdescomprimido: 0x%08lX\n\tcomprimido: 0x%08lX\n", tamanho_descomprimido, tamanho_comprimido);
	
	// Lê a seção comprimida
	bytes_lidos = fread(buffer_comprimido, tamanho_comprimido, 1, arq);
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
		free(buffer_comprimido);
		free(buffer_descomprimido);
		free(buffer_flags);
		exit(1);
	}
	
	// Lê a seção de flags
	bytes_lidos = fread(buffer_flags, tamanho_comprimido/8, 1, arq);
	// ATENÇÃO: Como é estimado o tamanho da seção, pode ultrapassar o fim do arquivo, mas pode não ser erro...
	if (bytes_lidos < 1)
	{
		fprintf(stderr, "Não conseguiu ler do arquivo\n");
		if (ferror (arq))
		{
			perror(nome_arquivo);
			fclose(arq);
			free(buffer_comprimido);
			free(buffer_descomprimido);
			free(buffer_flags);
			exit(1);
		}
		else if (feof (arq))
		{
			fprintf(stderr, "Atingiu o fim do arquivo\n");
			// Pode não ser erro...
		}
	}
	
	// TODO: Verificar a sanidade do que foi lido? Comparar o tamanho comprimido com os valores dos flags?
	//   bit 0 = 1 byte, 1 = 2bytes
	
	fclose(arq);
	
	// FIXME: Interpreta
	int posicao_leitura = 0;
	int posicao_escrita = 0;
	int posicao_flag = 0;
	char posicao_bit = 0;
	char flag;
	int posicao_retrocesso;
	int quantidade_retrocesso;
	
	memset(buffer_descomprimido, 0, tamanho_descomprimido);
	
	while (posicao_leitura < tamanho_comprimido)
	{
		// Pega o flag
		flag = buffer_flags[posicao_flag++];
		printf("Flag: %02hX (posicao: 0x%08X)\n", flag & 0x0FF, posicao_leitura);
		for (posicao_bit=0; posicao_bit < 8 && posicao_leitura < tamanho_comprimido; posicao_bit++)
		{
			printf("bit %d - ", posicao_bit);
			if ((flag & (1 << posicao_bit)) == 0)
			{
				printf("cru\n");
				// Não comprimido
				buffer_descomprimido[posicao_escrita++] = buffer_comprimido[posicao_leitura++];			
			}
			else
			{
				printf("comprimido: ");
				// Comprimido... inteiro de 16 bits em little-endian: primeiros 4 bits é o tamanho - 3, demais são o offset
				posicao_retrocesso = buffer_comprimido[posicao_leitura++];
				quantidade_retrocesso = (buffer_comprimido[posicao_leitura] & 0x0F0) >> 4;
				posicao_retrocesso |= (buffer_comprimido[posicao_leitura++] & 0x0F) << 8;
				quantidade_retrocesso += 3;
				
				posicao_retrocesso ++;
				
				printf("T: %d, Volta: %d\n", quantidade_retrocesso, posicao_retrocesso);
				
				int p;
				for (p = 0; p < quantidade_retrocesso; p++)
				{
					if (posicao_escrita - posicao_retrocesso < 0)
					{
						fprintf(stderr, "Erro na implementação/interpretação da descompressão!\n");
						free(buffer_comprimido);
						free(buffer_descomprimido);
						free(buffer_flags);
						exit(1);
					}
					buffer_descomprimido[posicao_escrita] = buffer_descomprimido[posicao_escrita - posicao_retrocesso];
					posicao_escrita++;
				}
			}
		}
	}
	
	printf("Fim da descompressão: abrindo/criando arquivo para gravação...\n");
	// Grava
	FILE * arq_saida = fopen(nome_arquivo_saida, "wb");
	if (arq_saida == NULL)
	{
		fprintf(stderr, "Não foi possível abrir o arquivo %s\n", nome_arquivo_saida);
		perror(nome_arquivo_saida);
		free(buffer_comprimido);
		free(buffer_descomprimido);
		free(buffer_flags);
		exit(1);
	}
	
	printf("Iniciando gravação...\n");
	if (fwrite(buffer_descomprimido, tamanho_descomprimido, 1, arq_saida) < 1)
	{
		fprintf(stderr, "Não foi possível gravar no arquivo %s\n", nome_arquivo_saida);
		perror(nome_arquivo_saida);
	
		fclose(arq_saida);
		free(buffer_comprimido);
		free(buffer_descomprimido);
		free(buffer_flags);
		exit(1);
	}
	fclose(arq_saida);

	printf("Fim da gravação...\n");
	free(buffer_comprimido);
	free(buffer_descomprimido);
	free(buffer_flags);
	return 0;
}

