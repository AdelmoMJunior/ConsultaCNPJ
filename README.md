# Consulta CNPJ

Aplicação feita para facilitar a consulta de CNPJ completo (Incluindo Inscrição estadual que a API da receita não disponibiliza) de empresas, integração simples por arquivo de texto (.txt).

# CONSULTA.EXE CNPJ(14) ARQ_DEST -> 

Ao chamar o executável com o paramêtro esperado, ele valida se o CNPJ é válido e consulta na API (CNPJ.WS) aguardando o retorno em JSON e tratando para .txt com formato predefinido no caminho indicado no segundo paramêtro.

# EXEMPLO DE RESPOSTA

01-CNPJ.......: 03748056000117
02-RAZAO......: AM SOFTWARE CONSULTORIA E SISTEMAS LTDA
03-FANTASIA...: AM CONSULTORIA
04-IE.........: 134.567.293
05-LOGRADOURO.: RUA A LOTEAMENTO QUINTA DO INGLES
06-NUMERO.....: 44
07-COMPLEMENTO: SALA 109
08-BAIRRO.....: CENTRO
09-CIDADE.....: Santo Antonio de Jesus
10-UF.........: BA
11-CEP........: 44571069
12-TELEFONE...: 07536310004
13-EMAIL......: am_software@hotmail.com
14-CODMUN.....: 2928703
