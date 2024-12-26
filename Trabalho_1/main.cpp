#include <iostream>
#include <vector>
#include <cstring>
#include <algorithm> // Para usar a função std::sort
#include <climits>   // Para constantes de limite

using namespace std;

int jobScheduling(vector<int> &startTime, vector<int> &endTime, vector<int> &profit); // função principal
int dfs(vector<vector<int>> &jobs, int pos); // lógica recursiva para calcular todas combinações possíveis de trabalhos
int findNextJob(vector<vector<int>>& jobs, int currentEndTime); // busca binária

// int main() {
//     // Exemplo de entrada   
//     vector<int> startTime = {1, 2, 3, 3};
//     vector<int> endTime = {3, 4, 5, 6};
//     vector<int> profit = {50, 10, 40, 70};

//     // Exibe o lucro máximo possível
//     cout << "Maximum Profit: " << jobScheduling(startTime, endTime, profit) << endl;
    
//     return 0;
// }

// Array global para armazenar os resultados (memorização)
int results[50001];
// Função principal que calcula o lucro máximo
int jobScheduling(vector<int> &startTime, vector<int> &endTime, vector<int> &profit) {
    // Inicializa o array 'results' com -1
    memset(results, -1, sizeof results);
    // cria a var para união dos vertores
    vector<vector<int>> jobs;
    for (int i = 0; i < startTime.size(); i++) {
        // Cada trabalho é representado como {startTime, endTime, profit}
        jobs.push_back({startTime[i], endTime[i], profit[i]});
    }
    // Ordena os trabalhos pelo tempo de início (startTime)
    sort(jobs.begin(), jobs.end());
    // Chama o DFS para calcular o lucro máximo começando do primeiro trabalho
    return dfs(jobs, 0);
}

// Função DFS para explorar combinações de trabalhos
int dfs(vector<vector<int>>& jobs, int pos) {
    // Caso base: se todos os trabalhos foram considerados
    if (pos >= jobs.size()) return 0;
    
    // Se o resultado para essa posição já foi calculado, retorna o valor memoizado
    if (results[pos] != -1) return results[pos];

    // Escolha 1: Pular o trabalho atual
    int skipCurrent = dfs(jobs, pos + 1);
    
    // Escolha 2: Incluir o trabalho atual
    int nextJobPos = findNextJob(jobs, jobs[pos][1]); // Encontra o próximo trabalho sem sobreposição
    int includeCurrent = jobs[pos][2] + dfs(jobs, nextJobPos);
    
    // Armazena o melhor resultado
    return results[pos] = max(includeCurrent, skipCurrent);
}

int findNextJob(vector<vector<int>>& jobs, int currentEndTime) {
    int left = 0, right = jobs.size(); // Inicializa o intervalo de busca
    while (left < right) {              // Enquanto houver intervalo a ser buscado
        int mid = left + (right - left) / 2; // Calcula o meio do intervalo
        if (jobs[mid][0] >= currentEndTime) {
            right = mid;   // Se o trabalho do meio começa depois ou no tempo atual, ajusta o limite superior
        } else {
            left = mid + 1; // Caso contrário, move o limite inferior para o próximo trabalho
        }
    }
    return left; // Retorna o índice do próximo trabalho que pode ser escolhido
}