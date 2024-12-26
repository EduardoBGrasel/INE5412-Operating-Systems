#include "fs.h"
#include <math.h>
#include <cstring> 
#include <algorithm>


int INE5412_FS::fs_format()
{
    // Verifica se o disco está montado
    // Uma tentativa de formatar um disco montado deve falhar
    if (get_mounted()) {
        cout << "ERROR: disk is mounted\n";
        return 0;
    }

    // Lê o superbloco
    union fs_block superblock;
    disk->read(0, superblock.data);

    // Libera a tabela de inodes
    for (int i = 0; i < superblock.super.ninodeblocks; i++) {
        // Lê o bloco de inodes
        union fs_block inode_block;
        disk->read(i + 1, inode_block.data);

        for (int j = 0; j < INODES_PER_BLOCK; j++) {
            // Marca o inode como inválido
            inode_block.inode[j].isvalid = 0;
        }

        // Escreve o bloco de inodes atualizado no disco
        disk->write(i + 1, inode_block.data);
    }

    // Calcula o número de blocos e reserva 10% para os inodes
    int total_blocks = disk->size();
    int inode_blocks = ceil(total_blocks * 0.1);

    // Configura os parâmetros do superbloco
    superblock.super.magic = FS_MAGIC;
    superblock.super.nblocks = total_blocks;
    superblock.super.ninodeblocks = inode_blocks;
    superblock.super.ninodes = inode_blocks * INODES_PER_BLOCK;

    // Escreve o superbloco atualizado no disco
    disk->write(0, superblock.data);

    return 1; // Retorna sucesso ao formatar o disco
}

void INE5412_FS::fs_debug()
{
	union fs_block block;
	// ler o superbloco
	disk->read(0, block.data);

	cout << "superblock:\n";
	cout << "    " << (block.super.magic == FS_MAGIC ? "magic number is valid\n" : "magic number is invalid!\n");
 	cout << "    " << block.super.nblocks << " blocks\n";
	cout << "    " << block.super.ninodeblocks << " inode blocks\n";
	cout << "    " << block.super.ninodes << " inodes\n";

	union fs_block inode_block;

	// percorre cada bloco de inode
	for (int i = 0; i < block.super.ninodeblocks; i++) {
		disk->read(i + 1, inode_block.data);

		// percorre cada inodo contido no bloco de inode
		for (int j = 0; j < INODES_PER_BLOCK; j++) {

			// recuperar o inode
			fs_inode inode = inode_block.inode[j];
			if (inode.isvalid) { // verificar sua validade
				int inode_number = (i * INODES_PER_BLOCK) + j; // calcular numero do inode
				// printar infos básicas
				cout << "inode " << inode_number << ":\n";
				cout << "    " << "size: " << inode.size << " bytes\n";

				// percorrer os blocos diretos
				cout << "    " << "direct blocks: "; 
				for (int direct_block : inode.direct) {
					if (direct_block) {
						cout << direct_block << " ";
					}
				}
				cout << "\n";

				// verificar se existe bloco indireto
				if (inode.indirect) {
					cout << "    " << "indirect block: " << inode.indirect << "\n";

					// recuperar o bloco indireto
					union fs_block indirect_block;
					disk->read(inode.indirect, indirect_block.data);
					
					// percorrer blocos indiretos
					cout << "    " << "indirect data blocks: ";
					for (int indirect_data_blocks : indirect_block.pointers) {
						if (indirect_data_blocks) {
							cout << indirect_data_blocks << " ";
						}
					}
					cout << "\n";
				}
			}
		}
	}
}

int INE5412_FS::fs_mount()
{
    // Lê o superbloco para verificar se há um sistema de arquivos
    union fs_block superblock;
    disk->read(0, superblock.data);

    // Verifica se o sistema de arquivos é válido
    if (superblock.super.magic != FS_MAGIC) {
        cerr << "file system invalid, format disk...\n";
        return 0;
    }

    // construção do bitmap
    int total_blocks = superblock.super.nblocks;
    bitmap.resize(total_blocks, 0);
    
    // Ocupa o superbloco no bitmap
    bitmap[0] = 1;

    // Processa os blocos de inodes
    for (int i = 0; i < superblock.super.ninodeblocks; i++) {
        // Ocupa os blocos de inodes no bitmap
        bitmap[i + 1] = 1;

        // Lê o bloco de inodes
        union fs_block inode_block;
        disk->read(i + 1, inode_block.data);

        // Processa os inodes do bloco
        for (int j = 0; j < INODES_PER_BLOCK; j++) {
            fs_inode &inode = inode_block.inode[j];

            // Verifica os blocos ocupados por inodes
            if (inode.isvalid) {
                // Marca os blocos diretos como ocupados no bitmap
                for (int direct_block : inode.direct) {
                    if (direct_block != 0) {
                        bitmap[direct_block] = 1;
                    }
                }

                // Verifica blocos indiretos
                if (inode.indirect) {
                    bitmap[inode.indirect] = 1; // Marca o bloco indireto como ocupado no bitmap

                    // Lê o bloco indireto
                    union fs_block indirect_block;
                    disk->read(inode.indirect, indirect_block.data);

                    // Marca os blocos apontados como ocupados no bitmap
                    for (int indirect_data_block : indirect_block.pointers) {
                        if (indirect_data_block != 0) {
                            bitmap[indirect_data_block] = 1;
                        }
                    }
                }
            }
        }
    }

    // Define o sistema de arquivos como montado
    set_mounted(true);
    return 1; // Retorna sucesso
}

int INE5412_FS::fs_create()
{
	// Certifica-se de que o disco está montado antes de prosseguir
	if (!get_mounted()) {
		cerr << "disk is not mounted\n";
		return 0;
	}

	union fs_block super_block;
	disk->read(0, super_block.data);

	union fs_block current_inode_block;

	// Itera pelos blocos de inodo disponíveis
	for (int block_index = 0; block_index < super_block.super.ninodeblocks; block_index++) {
		disk->read(block_index + 1, current_inode_block.data);

		// Percorre cada inodo dentro do bloco atual
		for (int inode_index = 1; inode_index <= INODES_PER_BLOCK; inode_index++) {

			fs_inode &current_inode = current_inode_block.inode[inode_index];

			// Verifica se o inodo está livre para uso
			if (!current_inode.isvalid) {
				// Calcula o identificador do inodo
				int inode_number = block_index * INODES_PER_BLOCK + inode_index;

				// Inicializa o inodo como válido e vazio
				current_inode.isvalid = 1;
				current_inode.indirect = 0;
				current_inode.size = 0;

				// Reseta os blocos diretos
				for (int &block : current_inode.direct) {
					block = 0;
				}

				// Persiste as alterações no disco
				inode_save(inode_number, &current_inode);

				// Retorna o número do inodo recém-criado
				return inode_number;
			}
		}
	}
	// Retorna falha caso não haja inodos disponíveis
	return 0;
}


int INE5412_FS::fs_delete(int inumber)
{
	// Verifica se o sistema de arquivos está montado
	if (!get_mounted()) {
		cerr << "disk is not mounted\n";
		return 0;
	}

	fs_inode inode;
	if (inode_load(inumber, &inode) && inode.isvalid) {

		// Libera os blocos diretos, verificando se estão alocados antes
		for (int &direct_block : inode.direct) {
			if (direct_block != 0) { // Verifica se o bloco está realmente alocado
				bitmap[direct_block] = 0; // Marca o bloco como livre no bitmap
				direct_block = 0;         // Limpa o bloco no inode
			}
		}

		// Caso necessário, libera os blocos indiretos
		if (inode.indirect) {
			union fs_block indirect_block;
			disk->read(inode.indirect, indirect_block.data);

			for (int &indirect_data_block : indirect_block.pointers) {
				if (indirect_data_block != 0) { // Verifica se o bloco indireto está alocado
					bitmap[indirect_data_block] = 0; // Marca o bloco como livre no bitmap
					indirect_data_block = 0;         // Limpa o bloco indireto
				}
			}

			// Libera o bloco que contém os ponteiros indiretos
			bitmap[inode.indirect] = 0; // Marca o bloco indireto como livre no bitmap
			inode.indirect = 0;         // Limpa o bloco indireto no inode
		}

		// Atualiza o tamanho e a validade do inode
		inode.isvalid = 0;
		inode.size = 0;

		// Salva o inode atualizado no disco
		inode_save(inumber, &inode);

		return 1; // Sucesso
	}
	return 0; // Falha
}


int INE5412_FS::fs_getsize(int inumber)
{
	// verifica se está montado
	if (!get_mounted()) {
		cerr << "disk is not mounted\n";
		return -1;
	}

	// carrega o inodo e retorna seu tamanho
	fs_inode inode;
	if (inode_load(inumber, &inode) && inode.isvalid) {
		return inode.size;
	} return -1;
}

int INE5412_FS::fs_read(int number, char *data, int length, int offset)
{
    // Verifica se o disco está montado
    if (!get_mounted()) {
        cerr << "disk is not mounted.\n";
        return 0;
    }

    // Carrega o inode correspondente ao número fornecido
    fs_inode inode;
    if (inode_load(number, &inode) && inode.isvalid) {
        int remaining_bytes = length;   // Quantidade de bytes restantes para leitura
        int total_bytes_read = 0;      // Total de bytes já lidos
        int block_number;              // Número do bloco correspondente ao deslocamento atual

        union fs_block current_block;

        // Lê os dados enquanto houver bytes restantes e o deslocamento estiver dentro do tamanho do inode
        while (remaining_bytes > 0 && offset < inode.size) {
            block_number = offset / Disk::DISK_BLOCK_SIZE; // Calcula o número do bloco correspondente

            if (block_number < POINTERS_PER_INODE) {
                // Lê os dados dos blocos diretos
                disk->read(inode.direct[block_number], current_block.data);
            } else {
                // Lê os dados dos blocos indiretos
                int indirect_index = block_number - POINTERS_PER_INODE;
                union fs_block indirect_block;
                disk->read(inode.indirect, indirect_block.data);
                disk->read(indirect_block.pointers[indirect_index], current_block.data);
            }

            // Calcula o deslocamento dentro do bloco e a quantidade de bytes a copiar
            int local_offset = offset % Disk::DISK_BLOCK_SIZE;
            int bytes_to_copy = min(Disk::DISK_BLOCK_SIZE - local_offset, remaining_bytes);
            bytes_to_copy = min(bytes_to_copy, inode.size - offset);

            // Copia os dados do bloco atual para o buffer de saída
            memcpy(data + total_bytes_read, current_block.data + local_offset, bytes_to_copy);

            // Atualiza os contadores e o deslocamento
            total_bytes_read += bytes_to_copy;
            remaining_bytes -= bytes_to_copy;
            offset += bytes_to_copy;
        }

        // Retorna o número total de bytes lidos
        return total_bytes_read;
    }

    // Retorna 0 caso o inode não seja válido ou falhe ao carregar
    return 0;
}


int INE5412_FS::fs_write(int inumber, const char *data, int length, int offset)
{
    // verifica se o disco está montado
    if (!get_mounted()) {
        cerr << "disk is not mounted.\n";
        return 0;
    }

    fs_inode inode;
    union fs_block current_block;
    union fs_block indirect_block;

    // carrega o inode correspondente ao inumber e verifica se ele é válido
    if (inode_load(inumber, &inode) && inode.isvalid) {
        int total_bytes_written = 0;  // Total de bytes já escritos
        int bytes_remaining = length; // Bytes que ainda precisam ser escritos

        while (bytes_remaining > 0) {
            int block_number = offset / Disk::DISK_BLOCK_SIZE; // Número do bloco baseado no deslocamento
            int local_offset = offset % Disk::DISK_BLOCK_SIZE; // Offset dentro do bloco atual

            int *target_block_pointer; // Ponteiro para o bloco que será usado (direto ou indireto)

            if (block_number < POINTERS_PER_INODE) {
                // Blocos diretos
                target_block_pointer = &inode.direct[block_number];
            } else {
                // Blocos indiretos
                if (!inode.indirect) {
                    // Aloca um bloco para o ponteiro indireto, caso não exista
                    int new_block = search_block();
                    if (new_block == -1) break; // Sem espaço disponível

                    inode.indirect = new_block;
                }

                // Lê o bloco indireto
                disk->read(inode.indirect, indirect_block.data);

                int indirect_index = block_number - POINTERS_PER_INODE;
                target_block_pointer = &indirect_block.pointers[indirect_index];

                // Aloca um bloco indireto, caso necessário
                if (!(*target_block_pointer)) {
                    int new_block = search_block();
                    if (new_block == -1) break; // Sem espaço disponível

                    *target_block_pointer = new_block;

                    // Atualiza o bloco indireto no disco
                    disk->write(inode.indirect, indirect_block.data);
                }
            }

            // Aloca um bloco direto, caso necessário
            if (!(*target_block_pointer)) {
                int new_block = search_block();
                if (new_block == -1) break; // Sem espaço disponível

                *target_block_pointer = new_block;
            }

            // Lê o bloco do disco para atualização
            disk->read(*target_block_pointer, current_block.data);

            // Determina quantos bytes podem ser copiados para o bloco atual
            int bytes_to_copy = std::min(bytes_remaining, Disk::DISK_BLOCK_SIZE - local_offset);

            // Copia os dados para o bloco atual
            memcpy(current_block.data + local_offset, data + total_bytes_written, bytes_to_copy);

            // Escreve o bloco atualizado no disco
            disk->write(*target_block_pointer, current_block.data);

            // Atualiza os contadores e deslocamentos
            total_bytes_written += bytes_to_copy;
            bytes_remaining -= bytes_to_copy;
            offset += bytes_to_copy;
        }

        // Atualiza o tamanho do inode, caso necessário
        if (inode.size < offset) {
            inode.size = offset;
            inode_save(inumber, &inode);
        }

        return total_bytes_written;
    }

    return 0;
}

int INE5412_FS::inode_load(int number, class fs_inode *inode) 
{
    union fs_block superblock;
    disk->read(0, superblock.data);

    // Verifica se o número do inode é válido
    if (number < 1 || number > superblock.super.ninodes) {
        cout << "Erro: Número do inode inválido.\n";
        return 0;
    }

    union fs_block inode_block;
    int inodes_per_block = INODES_PER_BLOCK; // Quantidade de inodes por bloco
    int inode_blocks_count = superblock.super.ninodeblocks; // Quantidade de blocos de inodes

    for (int block_index = 0; block_index < inode_blocks_count; ++block_index) {
        // Lê o bloco atual de inodes
        disk->read(block_index + 1, inode_block.data);

        for (int inode_index = 0; inode_index < inodes_per_block; ++inode_index) {
            int current_inode_number = block_index * inodes_per_block + inode_index;

            // Verifica se o inode atual corresponde ao número solicitado
            if (current_inode_number == number) {
                *inode = inode_block.inode[inode_index];
                return 1; // Retorna sucesso ao encontrar o inode
            }
        }
    }

    return 0; // Retorna falha caso o inode não seja encontrado
}


int INE5412_FS::inode_save(int number, class fs_inode *inode) 
{
    union fs_block superblock;
    disk->read(0, superblock.data);

    // Verifica se o número do inode é válido
    if (number < 1 || number > superblock.super.ninodes) {
        cerr << "Erro: Número do inode inválido.\n";
        return 0;
    }

    union fs_block inode_block;
    int inodes_per_block = INODES_PER_BLOCK; // Quantidade de inodes por bloco
    int inode_blocks_count = superblock.super.ninodeblocks; // Quantidade de blocos de inodes

    for (int block_index = 0; block_index < inode_blocks_count; ++block_index) {
        // Lê o bloco atual de inodes
        disk->read(block_index + 1, inode_block.data);

        for (int inode_index = 0; inode_index < inodes_per_block; ++inode_index) {
            int current_inode_number = block_index * inodes_per_block + inode_index;

            // Verifica se o inode atual corresponde ao número fornecido
            if (current_inode_number == number) {
                inode_block.inode[inode_index] = *inode; // Salva o inode no bloco
                disk->write(block_index + 1, inode_block.data); // Escreve o bloco atualizado no disco
                return 1; // Retorna sucesso após salvar
            }
        }
    }

    return 0; // Retorna falha caso o inode não seja salvo
}


// função auxiliar para encontrar um bloco livre
int INE5412_FS::search_block() 
{
    union fs_block superblock;
    disk->read(0, superblock.data);

    int total_blocks = superblock.super.nblocks; // Total de blocos disponíveis

    // Percorre todos os blocos para encontrar um bloco livre
    for (int block_index = 0; block_index < total_blocks; ++block_index) {
        if (!bitmap[block_index]) {
            bitmap[block_index] = 1; // Marca o bloco como ocupado no bitmap
            return block_index; // Retorna o índice do bloco livre encontrado
        }
    }

    return -1; // Retorna -1 caso nenhum bloco esteja disponível
}


void INE5412_FS::set_mounted(bool value) {
	mounted = value;
}
int INE5412_FS::get_mounted() {
	return mounted;
}