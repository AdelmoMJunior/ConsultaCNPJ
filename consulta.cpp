/*
 * SCRIPT C++ PARA CONSULTAR CNPJ
 *
 * Dependência de compilação: json.hpp (de nlohmann)
 * Coloque json.hpp no mesmo diretório ou em seu include path.
 */

#include <iostream>
#include <string>
#include <vector>
#include <regex>       // Para limpar_numeros
#include <fstream>     // Para salvar arquivo
#include <stdexcept>   // Para exceções
#include <numeric>     // Para std::accumulate (validar_cnpj)
#include <sstream>     // Para formatar string final
#include <algorithm>   // Para std::transform (to_lower)
#include <windows.h>
#include <wininet.h>   // Para HTTP (InternetOpen, etc.)
#include <winnls.h>     // Para NormalizeString e GetStringTypeW
#include "json.hpp"
#include <codecvt>
#include <locale>

#pragma comment(lib, "wininet.lib")    // Para WinINet
#pragma comment(lib, "normaliz.lib")   // Para NormalizeString
#pragma comment(lib, "user32.lib")     // Para MessageBoxW
#pragma comment(lib, "ws2_32.lib")     // Dependência de rede

// Para conveniência com a biblioteca JSON
using json = nlohmann::json;

std::wstring to_wstring_utf8(const std::string& s) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.from_bytes(s);
}

std::string WstringToString(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

std::wstring StringToWstring(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

void mostrar_msgbox(const std::string& titulo, const std::string& mensagem) {
    MessageBoxW(NULL, StringToWstring(mensagem).c_str(), StringToWstring(titulo).c_str(), MB_OK | MB_ICONERROR);
}

std::string limpar_numeros(const std::string& texto) {
    if (texto.empty()) return "";
    return std::regex_replace(texto, std::regex(R"(\D)"), "");
}

std::string safe_strip(std::string s) {
    s.erase(0, s.find_first_not_of(" \t\n\r\f\v"));
    s.erase(s.find_last_not_of(" \t\n\r\f\v") + 1);
    return s;
}

std::string remover_acentos(const std::string& texto_utf8) {
    if (texto_utf8.empty()) return "";

    std::wstring texto_w = StringToWstring(texto_utf8);

    int nfkd_len = NormalizeString(NormalizationKD, texto_w.c_str(), -1, NULL, 0);
    if (nfkd_len <= 0) {
        return texto_utf8;
    }
    
    std::wstring nfkd_form(nfkd_len, L'\0');
    nfkd_len = NormalizeString(NormalizationKD, texto_w.c_str(), -1, &nfkd_form[0], nfkd_len);
    
    if (nfkd_len > 0) {
        nfkd_form.resize(nfkd_len - 1);

        std::wstring texto_sem_acento_w;
        texto_sem_acento_w.reserve(nfkd_form.length());
        
        for (wchar_t c : nfkd_form) {
            WORD char_type[3];
            if (GetStringTypeW(CT_CTYPE3, &c, 1, &char_type[2])) {
                if (!(char_type[2] & C3_NONSPACING)) {
                    texto_sem_acento_w += c;
                }
            } else {
                texto_sem_acento_w += c;
            }
        }
        
        std::string texto_processado = WstringToString(texto_sem_acento_w);
        
        std::regex re_c("ç");
        texto_processado = std::regex_replace(texto_processado, re_c, "c");
        std::regex re_C("Ç");
        texto_processado = std::regex_replace(texto_processado, re_C, "C");
        
        return texto_processado;
    }
    
    // Se tudo falhar, retorna o original
    return texto_utf8;
}

std::string formatar_telefone(const std::string& ddd, const std::string& numero) {
    std::string tel = limpar_numeros(ddd + numero);
    if (tel.empty()) return "";
    if (tel.length() == 10) {
        return "0" + tel;
    } else if (tel.length() == 11) {
        return tel;
    }
    return tel;
}

bool validar_cnpj(const std::string& cnpj) {
    std::string numeros = limpar_numeros(cnpj);

    if (numeros.length() != 14) return false;

    bool all_same = true;
    for (size_t i = 1; i < numeros.length(); ++i) {
        if (numeros[i] != numeros[0]) {
            all_same = false;
            break;
        }
    }
    if (all_same) return false;

    auto calcular_digito = [](const std::string& cnpj_parcial, const std::vector<int>& pesos) -> std::string {
        int soma = 0;
        for (size_t i = 0; i < cnpj_parcial.length(); ++i) {
            soma += (cnpj_parcial[i] - '0') * pesos[i];
        }
        int resto = soma % 11;
        return (resto < 2) ? "0" : std::to_string(11 - resto);
    };

    std::vector<int> pesos1 = {5, 4, 3, 2, 9, 8, 7, 6, 5, 4, 3, 2};
    std::string digito1 = calcular_digito(numeros.substr(0, 12), pesos1);

    std::vector<int> pesos2 = {6, 5, 4, 3, 2, 9, 8, 7, 6, 5, 4, 3, 2};
    std::string digito2 = calcular_digito(numeros.substr(0, 12) + digito1, pesos2);

    return numeros.substr(14 - 2) == (digito1 + digito2);
}

json consultar_cnpj(const std::string& cnpj) {
    std::wstring url = L"https://publica.cnpj.ws/cnpj/" + StringToWstring(cnpj);
    std::string response_body;

    HINTERNET hInternet = InternetOpenW(
        L"CNPJ_Consulta_CPP/1.0",
        INTERNET_OPEN_TYPE_PRECONFIG,
        NULL,
        NULL,
        0
    );
    if (hInternet == NULL) {
        throw std::runtime_error("InternetOpenW falhou (Erro " + std::to_string(GetLastError()) + ")");
    }

    HINTERNET hConnect = InternetOpenUrlW(
        hInternet,
        url.c_str(),
        NULL,
        0,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE,
        0
    );
    if (hConnect == NULL) {
        InternetCloseHandle(hInternet);
        throw std::runtime_error("InternetOpenUrlW falhou. Verifique a URL e a conexão (Erro " + std::to_string(GetLastError()) + ")");
    }

    DWORD httpStatus = 0;
    DWORD statusSize = sizeof(httpStatus);
    if (!HttpQueryInfoW(hConnect, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &httpStatus, &statusSize, NULL)) {
         InternetCloseHandle(hConnect);
         InternetCloseHandle(hInternet);
         throw std::runtime_error("HttpQueryInfoW falhou (Erro " + std::to_string(GetLastError()) + ")");
    }

    if (httpStatus >= 400) {
        std::string errorMsg = "Erro na API (HTTP Status " + std::to_string(httpStatus) + ")";
         InternetCloseHandle(hConnect);
         InternetCloseHandle(hInternet);
         throw std::runtime_error(errorMsg);
    }

    char buffer[4096];
    DWORD bytesRead = 0;
    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        response_body.append(buffer, bytesRead);
    }

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    
    try {
        return json::parse(response_body);
    } catch (json::parse_error& e) {
        throw std::runtime_error("Falha ao decodificar JSON: " + std::string(e.what()));
    }
}

std::string json_str_safe(const json& j, const std::string& key) {
    if (j.contains(key) && j[key].is_string())
        return j[key].get<std::string>();
    return "";
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        MessageBoxW(nullptr, L"Uso: programa.exe <CNPJ> <NOME_ARQUIVO>", L"Erro", MB_ICONERROR);
        return 1;
    }

    std::string cnpj_arg = argv[1];
    std::string nome_arquivo = argv[2];
    std::string cnpj_limpo = limpar_numeros(cnpj_arg);

    if (cnpj_limpo.length() != 14) {
        mostrar_msgbox("CNPJ inválido", "O CNPJ '" + cnpj_arg + "' não possui 14 dígitos válidos.");
        return 1;
    }

    if (!validar_cnpj(cnpj_limpo)) {
        mostrar_msgbox("CNPJ inválido", "O CNPJ '" + cnpj_arg + "' não é válido.");
        return 1;
    }

    json dados;
    try {
        dados = consultar_cnpj(cnpj_limpo);
    } catch (const std::exception& e) {
        mostrar_msgbox("Erro de conexão", "Não foi possível consultar o CNPJ.\n\nDetalhes: " + std::string(e.what()));
        return 1;
    }

    if (dados.is_null() || !dados.contains("estabelecimento")) {
        mostrar_msgbox("CNPJ inválido", "O CNPJ '" + cnpj_limpo + "' não foi encontrado na base pública.");
        return 1;
    }

    // --- Extração dos dados ---
    json est = dados.value("estabelecimento", json::object());
    std::string ie = "";

    if (est.contains("inscricoes_estaduais") && est["inscricoes_estaduais"].is_array() && !est["inscricoes_estaduais"].empty()) {
        json ie_obj = est["inscricoes_estaduais"][0];
        ie = limpar_numeros(json_str_safe(ie_obj, "inscricao_estadual"));
    }

    std::string tipo_logradouro = safe_strip(json_str_safe(est, "tipo_logradouro"));
    std::string logradouro      = safe_strip(json_str_safe(est, "logradouro"));
    std::string ddd1            = safe_strip(json_str_safe(est, "ddd1"));
    std::string telefone1       = safe_strip(json_str_safe(est, "telefone1"));
    std::string telefone        = formatar_telefone(ddd1, telefone1);
    std::string email           = safe_strip(json_str_safe(est, "email"));

    auto to_lower = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    };

    std::string logradouro_completo;
    if (!tipo_logradouro.empty() && !logradouro.empty() &&
        to_lower(logradouro).find(to_lower(tipo_logradouro)) == std::string::npos)
    {
        logradouro_completo = tipo_logradouro + " " + logradouro;
    } else {
        logradouro_completo = !tipo_logradouro.empty() ? tipo_logradouro : logradouro;
    }

    std::string cidade_nome = "";
    if (est.contains("cidade") && est["cidade"].is_object()) {
        cidade_nome = safe_strip(json_str_safe(est["cidade"], "nome"));
    }

    std::string estado_sigla = "";
    if (est.contains("estado") && est["estado"].is_object()) {
        estado_sigla = safe_strip(json_str_safe(est["estado"], "sigla"));
    }

    std::string cod_mun = "";
    if (est.contains("cidade") && est["cidade"].is_object()) {
        auto& cidade = est["cidade"];
        if (cidade.contains("ibge_id")) {
            if (cidade["ibge_id"].is_number_integer())
                cod_mun = std::to_string(cidade["ibge_id"].get<int>());
            else if (cidade["ibge_id"].is_string())
                cod_mun = cidade["ibge_id"].get<std::string>();
        }
    }

    std::stringstream ss;
    ss << "01-CNPJ.......: " << limpar_numeros(json_str_safe(est, "cnpj")) << "\r\n"
       << "02-RAZAO......: " << safe_strip(json_str_safe(dados, "razao_social")) << "\r\n"
       << "03-FANTASIA...: " << safe_strip(json_str_safe(est, "nome_fantasia")) << "\r\n"
       << "04-IE.........: " << ie << "\r\n"
       << "05-LOGRADOURO.: " << logradouro_completo << "\r\n"
       << "06-NUMERO.....: " << safe_strip(json_str_safe(est, "numero")) << "\r\n"
       << "07-COMPLEMENTO: " << safe_strip(json_str_safe(est, "complemento")) << "\r\n"
       << "08-BAIRRO.....: " << safe_strip(json_str_safe(est, "bairro")) << "\r\n"
       << "09-CIDADE.....: " << cidade_nome << "\r\n"
       << "10-UF.........: " << estado_sigla << "\r\n"
       << "11-CEP........: " << limpar_numeros(json_str_safe(est, "cep")) << "\r\n"
       << "12-TELEFONE...: " << telefone << "\r\n"
       << "13-EMAIL......: " << email << "\r\n"
       << "14-CODMUN.....: " << cod_mun << "\r\n";

    std::string texto_final = remover_acentos(ss.str());
    std::string texto_final_crlf;
    texto_final_crlf.reserve(texto_final.size() * 2);

    for (size_t i = 0; i < texto_final.size(); ++i) {
        if (texto_final[i] == '\n' && (i == 0 || texto_final[i - 1] != '\r')) {
            texto_final_crlf += "\r\n";
        } else {
            texto_final_crlf += texto_final[i];
        }
    }

    try {
        std::wstring nome_arquivo_w = StringToWstring(nome_arquivo);
        std::ofstream f(nome_arquivo_w.c_str(), std::ios::out | std::ios::binary);
        if (!f.is_open()) {
            throw std::runtime_error("Não foi possível abrir o arquivo para escrita.");
        }
        f.write(texto_final_crlf.c_str(), texto_final_crlf.length());
        f.close();
    } catch (const std::exception& e) {
        mostrar_msgbox("Erro ao salvar", "Não foi possível salvar o arquivo.\n\nDetalhes: " + std::string(e.what()));
        return 1;
    }

    return 0;
}

