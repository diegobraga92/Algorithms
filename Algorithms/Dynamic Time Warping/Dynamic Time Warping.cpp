#include <iostream>
#include <vector>
#include <array>
#include <limits>
#include <fstream>
#include <sstream>
#include <ctime>

// Enum para selecionar qual tipo de distância vai ser calculada, usado para passar como parâmetro
enum class TIPO_DTW
{
	DTW = 0,
	DTW_I = 1,
	DTW_D = 2
};

// Struct para armazenar a posicao no teste 3D
struct posicao
{
	double x = 0;
	double y = 0;
	double z = 0;

	posicao(const double& x_, const double& y_, const double& z_)
	{
		x = x_;
		y = y_;
		z = z_;
	}
};

// Struct para facilitar manipulação das series
struct serie
{
	std::vector<posicao> posicoes;
	int tipo;

	int size() { return (int)posicoes.size(); }
	void clear() { posicoes.clear(); }
	void push_back(const posicao& pos) { posicoes.push_back(pos); }
	posicao& operator [](const int& i) { return posicoes[i]; }
};

// Matriz DTW, tamanho 200x200 para cobrir todos os casos de teste
std::vector<std::vector<double>> matrizDTW(200, std::vector<double>(200));

// Matrizes para leitura dos arquivos
std::vector<serie> matrizTreino;
std::vector<serie> matrizTeste;
std::vector<serie> matrizTreino3D;
std::vector<serie> matrizTeste3D;

// Calculo da distância de acordo com o tipo passado, se DTW_I, o eixo define qual dimensão será calculada
double distancia(posicao& posA, posicao& posB, const TIPO_DTW& tipo, const int& eixo = 0)
{
	if (tipo == TIPO_DTW::DTW || (tipo == TIPO_DTW::DTW_I && eixo == 0))
		return pow(posA.x - posB.x, 2);
	else if (tipo == TIPO_DTW::DTW_I && eixo == 1)
		return pow(posA.y - posB.y, 2);
	else if (tipo == TIPO_DTW::DTW_I && eixo == 2)
		return pow(posA.z - posB.z, 2);
	else // DTW_D
		return pow(posA.x - posB.x, 2) + pow(posA.y - posB.y, 2) + pow(posA.z - posB.z, 2);
}

// Função para calcular DTW
double distanciaDTW(serie& serieRef, serie& serieComp, const double& limite = 10, const TIPO_DTW& tipo = TIPO_DTW::DTW, const int& eixo = 0)
{
	// Pega tamanhos e calcula banda
	int sizeRef = (int)serieRef.size();
	int sizeComp = (int)serieComp.size();

	// Calcula a banda,
	int banda = (int)std::ceil(sizeRef * limite);
	// Garante que a banda seja maior ou igual à diferença de tamanho das duas series
	//banda = std::max(banda, std::abs(sizeRef - sizeComp));

	// Atribui a região da matriz que será usada com valor infinito
	for (auto x = 0; x < sizeRef; x++)
	{
		for (auto y = 0; y < sizeComp; y++)
			matrizDTW[x][y] = std::numeric_limits<double>::max();
	}
	matrizDTW[0][0] = 0;

	// Completa a matriz
	for (auto posRef = 1; posRef < sizeRef; posRef++)
	{
		for (auto posComp = std::max(1, posRef - banda); posComp < std::min(sizeComp, posRef + banda + 1); posComp++)
		{
			matrizDTW[posRef][posComp] = distancia(serieRef[posRef], serieComp[posComp], tipo, eixo) +
				std::min(matrizDTW[posRef - 1][posComp], std::min(matrizDTW[posRef][posComp - 1],
					matrizDTW[posRef - 1][posComp - 1]));
		}
	}

	return matrizDTW[sizeRef - 1][sizeComp - 1];
}

// Função para ler os arquivos de teste
void leArquivo(std::ifstream& arquivo, std::vector<serie>& matrizParaArmazenar, const bool& leitura3D = false)
{
	double leituraX, leituraY, leituraZ;
	std::string leitura;
	serie linhaAtual;

	while (std::getline(arquivo, leitura))
	{
		linhaAtual.clear();
		std::istringstream iss(leitura);
		iss >> linhaAtual.tipo;
		if (!leitura3D) // Leitura 1D
		{
			while (iss >> leituraX)
				linhaAtual.push_back(posicao(leituraX, 0, 0));
		}
		else // Leitura 3D
		{
			while (iss >> leituraX >> leituraY >> leituraZ)
				linhaAtual.push_back(posicao(leituraX, leituraY, leituraZ));
		}
		matrizParaArmazenar.push_back(linhaAtual);
	}
}

int main()
{
	std::clock_t begin, end;

	////////////////////////  Entrada  ///////////////////////////
	// Leitura dos arquivos treino e teste
	std::ifstream arqTreino("treino.txt");
	std::ifstream arqTeste("teste.txt");
	std::ifstream arqTreino3D("treino3D.txt");
	std::ifstream arqTeste3D("teste3D.txt");

	leArquivo(arqTreino, matrizTreino);
	leArquivo(arqTeste, matrizTeste);
	leArquivo(arqTreino3D, matrizTreino3D, true);
	leArquivo(arqTeste3D, matrizTeste3D, true);

	//////////////////////  Processamento  ////////////////////////
	double menorDistancia, distanciaAtual, porcentagemAcertos, tempoDeProcessamento;
	int menorTipo, acertos = 0;

	// Define os tipos de cálculo de distância que serão usados
	std::vector<TIPO_DTW> tiposTeste = { TIPO_DTW::DTW, TIPO_DTW::DTW_I, TIPO_DTW::DTW_D };

	// Define as porcentagens de banda que serão testadas, sendo o valor 10 == sem banda
	std::vector<double> bandasParaTeste = { 0,0.01,0.05,0.1,0.2,0.5,1,10 };

	// Executa os testes com cada tipo de DTW
	for (TIPO_DTW& tipo : tiposTeste)
	{
		switch (tipo) {
		case TIPO_DTW::DTW: std::cout << "Testes 1D DTW ----------------------- \n"; break;
		case TIPO_DTW::DTW_I: std::cout << "\nTestes 3D DTW_I ----------------------- \n"; break;
		case TIPO_DTW::DTW_D: std::cout << "\nTestes 3D DTW_D ----------------------- \n"; break;
		}
		// Executa o teste com cada largura de banda
		for (double& banda : bandasParaTeste)
		{
			begin = clock();
			acertos = 0;
			std::cout << "Tamanho da Banda: " << banda * 100 << "% -- ";

			for (serie& serieTeste : (tipo == TIPO_DTW::DTW ? matrizTeste : matrizTeste3D))
			{
				menorDistancia = std::numeric_limits<double>::max();
				for (serie& serieTreino : (tipo == TIPO_DTW::DTW ? matrizTreino : matrizTreino3D))
				{
					if (tipo == TIPO_DTW::DTW_I)
					{
						distanciaAtual = distanciaDTW(serieTeste, serieTreino, banda, tipo, 0);
						distanciaAtual += distanciaDTW(serieTeste, serieTreino, banda, tipo, 1);
						distanciaAtual += distanciaDTW(serieTeste, serieTreino, banda, tipo, 2);
					}
					else // Calculo para DTW e DTWD
						distanciaAtual = distanciaDTW(serieTeste, serieTreino, banda, tipo);

					if (distanciaAtual < menorDistancia)
					{
						menorDistancia = distanciaAtual;
						menorTipo = (int)serieTreino.tipo;
					}
				}

				if ((int)serieTeste.tipo == menorTipo)
					acertos++;
			}

			end = clock();
			tempoDeProcessamento = double(end - begin) / CLOCKS_PER_SEC;
			if (tipo == TIPO_DTW::DTW)
				porcentagemAcertos = acertos / (double)matrizTeste.size();
			else
				porcentagemAcertos = acertos / (double)matrizTeste3D.size();
			std::cout << "Porcentagem de acerto: " << porcentagemAcertos * 100 << "% -- ";
			std::cout << "Tempo de Processamento: " << tempoDeProcessamento << "\n";
		}
	}

	std::getchar();
	std::getchar();

	return 0;
}