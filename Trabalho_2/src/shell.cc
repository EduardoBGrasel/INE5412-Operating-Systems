#include "fs.h"
#include "disk.h"
#include <SFML/Graphics.hpp>
#include <thread>
#include <functional>
#include <atomic>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class File_Ops
{
public:
    static int do_copyin(const char *filename, int inumber, INE5412_FS *fs);

    static int do_copyout(int inumber, const char *filename, INE5412_FS *fs);

};

using namespace std;

// Definição da classe Button e da interface gráfica (não altera nada da lógica do terminal)

namespace GRAPHIC_INTERFACE {
	std::atomic<bool> running(true); // Variável de sinalização para controle da thread
    class Button {
    public:
        sf::RectangleShape shape;
        sf::Text text;
        std::function<void()> onClick;

        Button(const sf::Vector2f& position, const sf::Vector2f& size, const std::string& label, sf::Font& font, std::function<void()> onClick)
            : onClick(onClick) {
            shape.setSize(size);
            shape.setPosition(position);
            shape.setFillColor(sf::Color::Blue);

            text.setFont(font);
            text.setString(label);
            text.setCharacterSize(16);
            text.setFillColor(sf::Color::White);
            text.setPosition(
                position.x + size.x / 2 - text.getGlobalBounds().width / 2,
                position.y + size.y / 2 - text.getGlobalBounds().height / 2
            );
        }

        void draw(sf::RenderWindow& window) {
            window.draw(shape);
            window.draw(text);
        }

        bool isMouseOver(sf::Vector2i mousePosition) {
            return shape.getGlobalBounds().contains(sf::Vector2f(mousePosition));
        }
    };


    void run(INE5412_FS* fs) {
        sf::RenderWindow window(sf::VideoMode(400, 600), "SimpleFS Interface");
        sf::Font font;
        if (!font.loadFromFile("arial.ttf")) {
            std::cerr << "Failed to load font\n";
            return;
        }

        // Botões
        std::vector<Button> buttons;

		// Botão Format
        buttons.emplace_back(sf::Vector2f(50, 50), sf::Vector2f(300, 40), "Format", font, [fs]() {
            if (fs->fs_format()) {
                // MessageBox de sucesso (ou similar)
                system("zenity --info --text=\"Disco formatado com sucesso!\"");
            } else {
                // MessageBox de erro (ou similar)
                system("zenity --info --text=\"Não foi possivel formatar o disco!\"");
            }
        });

		// Botão Mount
		buttons.emplace_back(sf::Vector2f(50, 100), sf::Vector2f(300, 40), "Mount", font, [fs]() {
            if (fs->fs_mount()) {
                // MessageBox de sucesso (ou similar)
               system("zenity --info --text=\"Disco montado com sucesso!\"");
            } else {
                // MessageBox de erro (ou similar)
                system("zenity --info --text=\"Não foi possivel montar o disco!\"");
            }
        });

        // Botão Debug
        buttons.emplace_back(sf::Vector2f(50, 150), sf::Vector2f(300, 40), "Debug", font, [fs]() {
            fs->fs_debug();
			system("zenity --info --text=\"Veja as informações no terminal!\"");
        });

		// Botão criar Nodo
		buttons.emplace_back(sf::Vector2f(50, 200), sf::Vector2f(300, 40), "Create", font, [fs]() {
			if (fs->fs_create()) {
				system("zenity --info --text=\"Inode criado com sucesso!\"");
			} else {
				system("zenity --info --text=\"Não foi possível criar o Inode!\"");
			}
		});

		// Botão Delete
		buttons.emplace_back(sf::Vector2f(50, 250), sf::Vector2f(300, 40), "Delete", font, [fs]() {
		// Solicita ao usuário o número do inode
		char buffer[128];
		FILE* pipe = popen("zenity --entry --title=\"Delete\" --text=\"Informe o número do inode a ser deletado:\" 2>/dev/null", "r");
		if (pipe) {
			fgets(buffer, sizeof(buffer), pipe);
			pclose(pipe);
		}

		// Remove o caractere de nova linha da entrada
		std::string input(buffer);
		input.erase(std::remove(input.begin(), input.end(), '\n'), input.end());

		// Tenta converter a entrada para um número inteiro
		try {
			int inode = std::stoi(input);

			// Chama a função fs_delete
			if (fs->fs_delete(inode)) {
				// MessageBox de sucesso
				system("zenity --info --text=\"Inode deletado com sucesso!\"");
			} else {
				// MessageBox de erro
				system("zenity --error --text=\"Não foi possível deletar o inode!\"");
			}
			} catch (const std::exception&) {
				// MessageBox de erro para entrada inválida
				system("zenity --error --text=\"Entrada inválida! Certifique-se de informar um número.\"");
			}
		});

		// Botão Cat
		buttons.emplace_back(sf::Vector2f(50, 300), sf::Vector2f(300, 40), "Cat", font, [fs]() {
			// Solicita ao usuário o número do inode
			char buffer[128];
			FILE* pipe = popen("zenity --entry --title=\"Cat\" --text=\"Informe o número do inode para exibir o conteúdo:\" 2>/dev/null", "r");
			if (pipe) {
				fgets(buffer, sizeof(buffer), pipe);
				pclose(pipe);
			}

			// Remove o caractere de nova linha da entrada
			std::string input(buffer);
			input.erase(std::remove(input.begin(), input.end(), '\n'), input.end());

			// Tenta converter a entrada para um número inteiro
			try {
				int inode = std::stoi(input);

				// Chama a função do_copyout para exibir o conteúdo no terminal
				if (File_Ops::do_copyout(inode, "/dev/stdout", fs)) {
					// Mensagem de sucesso
					system("zenity --info --text=\"Conteúdo do inode exibido no terminal.\"");
				} else {
					// Mensagem de erro
					system("zenity --error --text=\"Não foi possível exibir o conteúdo do inode.\"");
				}
			} catch (const std::exception&) {
				// Mensagem de erro para entrada inválida
				system("zenity --error --text=\"Entrada inválida! Certifique-se de informar um número.\"");
			}
		});

		// Botão CopyIn
		buttons.emplace_back(sf::Vector2f(50, 350), sf::Vector2f(300, 40), "CopyIn", font, [fs]() {
			// Solicita o caminho do arquivo ao usuário
			char filePathBuffer[256];
			FILE* filePathPipe = popen("zenity --entry --title=\"CopyIn\" --text=\"Informe o caminho do arquivo:\" 2>/dev/null", "r");
			if (filePathPipe) {
				fgets(filePathBuffer, sizeof(filePathBuffer), filePathPipe);
				pclose(filePathPipe);
			}

			// Remove o caractere de nova linha da entrada do caminho do arquivo
			std::string filePath(filePathBuffer);
			filePath.erase(std::remove(filePath.begin(), filePath.end(), '\n'), filePath.end());

			// Solicita o número do inode ao usuário
			char inodeBuffer[128];
			FILE* inodePipe = popen("zenity --entry --title=\"CopyIn\" --text=\"Informe o número do inode:\" 2>/dev/null", "r");
			if (inodePipe) {
				fgets(inodeBuffer, sizeof(inodeBuffer), inodePipe);
				pclose(inodePipe);
			}

			// Remove o caractere de nova linha da entrada do inode
			std::string inodeInput(inodeBuffer);
			inodeInput.erase(std::remove(inodeInput.begin(), inodeInput.end(), '\n'), inodeInput.end());

			// Tenta converter a entrada para um número inteiro
			try {
				int inode = std::stoi(inodeInput);

				// Chama a função do_copyin para copiar o arquivo para o inode
				if (File_Ops::do_copyin(filePath.c_str(), inode, fs)) {
					// Mensagem de sucesso
					system("zenity --info --text=\"Arquivo copiado para o inode com sucesso!\"");
				} else {
					// Mensagem de erro
					system("zenity --error --text=\"Não foi possível copiar o arquivo para o inode.\"");
				}
			} catch (const std::exception&) {
				// Mensagem de erro para entrada inválida
				system("zenity --error --text=\"Entrada inválida! Certifique-se de informar um número válido para o inode.\"");
			}
		});

		// botão copyout
		buttons.emplace_back(sf::Vector2f(50, 400), sf::Vector2f(300, 40), "Copyout", font, [fs]() {
			// Janela para entrada do número do inode
			system("zenity --entry --text=\"Digite o número do inode:\" --entry-text=\"\" > /tmp/inode_input.txt");
			std::ifstream inodeFile("/tmp/inode_input.txt");
			std::string inodeStr;
			std::getline(inodeFile, inodeStr);
			inodeFile.close();

			// Janela para entrada do caminho do arquivo
			system("zenity --file-selection --save --title=\"Escolha o local para salvar o arquivo\" > /tmp/file_output.txt");
			std::ifstream filePathFile("/tmp/file_output.txt");
			std::string filePath;
			std::getline(filePathFile, filePath);
			filePathFile.close();

			// Validar entradas
			if (inodeStr.empty() || filePath.empty()) {
				system("zenity --error --text=\"Entrada inválida! O número do inode ou o caminho do arquivo não foram fornecidos.\"");
				return;
			}

			int inode = std::stoi(inodeStr); // Converter o inode para inteiro
			if (File_Ops::do_copyout(inode, filePath.c_str(), fs)) {
				// Mensagem de sucesso
				system("zenity --info --text=\"Arquivo copiado do inode para o arquivo com sucesso!\"");
			} else {
				// Mensagem de erro
				system("zenity --error --text=\"Não foi possível copiar o inode para o arquivo.\"");
			}
		});

		buttons.emplace_back(sf::Vector2f(50, 450), sf::Vector2f(300, 40), "Exit", font, []() {
			// Exibe uma mensagem de confirmação antes de sair
			int confirmation = system("zenity --question --text=\"Tem certeza de que deseja sair?\" --width=300");

			// Código de retorno 0 indica que o usuário confirmou
			if (confirmation == 0) {
				exit(0); // Encerra o programa
			}
		});


        while (window.isOpen() && running) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                } else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2i mousePosition = sf::Mouse::getPosition(window);
                    for (auto& button : buttons) {
                        if (button.isMouseOver(mousePosition)) {
                            button.onClick();
                        }
                    }
                }
            }

            window.clear();
            for (auto& button : buttons) {
                button.draw(window);
            }
            window.display();
        }
    }
}  // namespace GRAPHIC_INTERFACE

int main( int argc, char *argv[] )
{
	char line[1024];
	char cmd[1024];
	char arg1[1024];
	char arg2[1024];
	int inumber, result, args;

	if(argc != 3) {
		cout << "use: " << argv[0] << " <diskfile> <nblocks>\n";
		return 1;
	}


    Disk disk(argv[1], atoi(argv[2]));

    INE5412_FS fs(&disk);
	std::thread sfmlThread(std::bind(GRAPHIC_INTERFACE::run, &fs));

	cout << "opened emulated disk image " << argv[1] << " with " << disk.size() << " blocks\n";

	while(1) {
		cout << " simplefs> ";
		fflush(stdout);

		if(!fgets(line,sizeof(line),stdin)) 
            break;

		if(line[0] == '\n') 
            continue;

		line[strlen(line)-1] = 0;

		args = sscanf(line,"%s %s %s", cmd, arg1, arg2);

		if(args == 0) 
            continue;

		if(!strcmp(cmd, "format")) {
			if(args == 1) {
				if(fs.fs_format()) {
					cout << "disk formatted.\n";
				} else {
					cout << "format failed!\n";
				}
			} else {
				cout << "use: format\n";
			}
		} else if(!strcmp(cmd, "mount")) {
			if(args == 1) {
				if(fs.fs_mount()) {
					cout << "disk mounted.\n";
				} else {
					cout << "mount failed!\n";
				}
			} else {
				cout << "use: mount\n";
			}
		} else if(!strcmp(cmd, "debug")) {
			if(args == 1) {
				fs.fs_debug();
			} else {
				cout << "use: debug\n";
			}
		} else if(!strcmp(cmd, "getsize")) {
			if(args == 2) {
				inumber = atoi(arg1);
				result = fs.fs_getsize(inumber);
				if(result >= 0) {
					cout << "inode " << inumber << " has size " << result << "\n";
				} else {
					cout << "getsize failed!\n";
				}
			} else {
				cout << "use: getsize <inumber>\n";
			}
			
		} else if(!strcmp(cmd, "create")) {
			if(args == 1) {
				inumber = fs.fs_create();
				if(inumber > 0) {
					cout << "created inode " << inumber << "\n";
				} else {
					cout << "create failed!\n";
				}
			} else {
				cout << "use: create\n";
			}
		} else if(!strcmp(cmd, "delete")) {
			if(args == 2) {
				inumber = atoi(arg1);
				if(fs.fs_delete(inumber)) {
					cout << "inode " << inumber << " deleted.\n";
				} else {
					cout << "delete failed!\n";	
				}
			} else {
				cout << "use: delete <inumber>\n";
			}
		} else if(!strcmp(cmd, "cat")) {
			if(args==2) {
				inumber = atoi(arg1);
				if(!File_Ops::do_copyout(inumber, "/dev/stdout", &fs)) {
					cout << "cat failed!\n";
				}
			} else {
				cout << "use: cat <inumber>\n";
			}

		} else if(!strcmp(cmd,"copyin")) {
			if(args==3) {
				inumber = atoi(arg2);
				if(File_Ops::do_copyin(arg1, inumber, &fs)) {
					cout << "copied file " << arg1 << " to inode " << inumber << "\n";
				} else {
					cout << "copy failed!\n";
				}
			} else {
				cout << "use: copyin <filename> <inumber>\n";
			}

		} else if(!strcmp(cmd, "copyout")) {
			if(args == 3) {
				inumber = atoi(arg1);
				if(File_Ops::do_copyout(inumber, arg2, &fs)) {
					cout << "copied inode " << inumber << " to file " << arg2 << "\n";
				} else {
					cout << "copy failed!\n";
				}
			} else {
				cout << "use: copyout <inumber> <filename>\n";
			}

		} else if(!strcmp(cmd, "help")) {
			cout << "Commands are:\n";
			cout << "    format\n";
			cout << "    mount\n";
			cout << "    debug\n";
			cout << "    create\n";
			cout << "    delete  <inode>\n";
			cout << "    cat     <inode>\n";
			cout << "    copyin  <file> <inode>\n";
			cout << "    copyout <inode> <file>\n";
			cout << "    help\n";
			cout << "    quit\n";
			cout << "    exit\n";
		} else if(!strcmp(cmd, "quit")) {
			GRAPHIC_INTERFACE::running = false; // Envia o sinal para a thread SFML finalizar
    		sfmlThread.join(); // Aguarda a thread SFML terminar
    		break; // Sai do loop principal
		} else if(!strcmp(cmd, "exit")) {
			GRAPHIC_INTERFACE::running = false; // Envia o sinal para a thread SFML finalizar
    		sfmlThread.join(); // Aguarda a thread SFML terminar
    		break; // Sai do loop principal
		} else {
			cout << "unknown command: " << cmd << "\n";
			cout << "type 'help' for a list of commands.\n";
			result = 1;
		}
	}

	cout << "closing emulated disk.\n";
	disk.close();

	return 0;
}

int File_Ops::do_copyin(const char *filename, int inumber, INE5412_FS *fs)
{
	FILE *file;
	int offset=0, result, actual;
	char buffer[16384];

	file = fopen(filename, "r");
	if(!file) {
		cout << "couldn't open " << filename << "\n";
		return 0;
	}

	while(1) {
		result = fread(buffer,1,sizeof(buffer),file);
		if(result <= 0) break;
		if(result > 0) {
			actual = fs->fs_write(inumber,buffer,result,offset);
			if(actual<0) {
				cout << "ERROR: fs_write return invalid result " << actual << "\n";
				break;
			}
			offset += actual;
			if(actual!=result) {
				cout << "WARNING: fs_write only wrote " << actual << " bytes, not " << result << " bytes\n";
				break;
			}
		}
	}

	cout << offset << " bytes copied\n";

    fclose(file);

	return 1;
}

int File_Ops::do_copyout(int inumber, const char *filename, INE5412_FS *fs)
{
	FILE *file;
	int offset = 0, result;
	char buffer[16384];

	file = fopen(filename,"w");
	if(!file) {
		cout << "couldn't open " << filename << "\n";
		return 0;
	}

	while(1) {
		result = fs->fs_read(inumber,buffer,sizeof(buffer),offset);
		if(result<=0) break;
		fwrite(buffer,1,result,file);
		offset += result;
	}

	cout << offset << " bytes copied\n";

	fclose(file);
	return 1;
}

