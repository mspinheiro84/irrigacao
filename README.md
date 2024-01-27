# Projeto de irrigação

cliente precisava de um sistema de irrigação para os oito vasos em seu apartamento, localizados em uma área coberta onde as plantas não recebem chuva. Para garantir que as plantas não fiquem sem água quando o cliente estiver viajando, foi sugerido o uso de um reservatório de água com uma bomba para a irrigação. Dessa forma, ao fechar o registro geral da água durante a viagem, as plantas continuarão a receber a quantidade necessária de água para se manterem saudáveis.

## Sistema

O sistema deve ser capaz de receber os dados do agendamento da irrigação, incluindo o horário, a duração da irrigação e quais dos oito vasos serão irrigados. Esses dados serão armazenados para permitir a execução automatizada no horário programado. Ao chegar no horário agendado, o sistema abrirá as válvulas correspondentes e monitorará o fluxo de água através de um sensor de vazão. Caso o sensor não detecte a presença de água no sistema, a bomba será ativada para suprir a demanda. Se, mesmo com a bomba em funcionamento, não houver fluxo de água detectado, a bomba será desligada e as válvulas serão fechadas como medida de segurança para proteger o equipamento.

## Como usar

Durante a primeira inicialização, o sistema exigirá uma conexão com a rede Wi-Fi para operar. Para facilitar esse processo, será gerada uma rede Wi-Fi aberta, permitindo que o usuário se conecte e forneça as credenciais de sua rede doméstica. Caso seja necessário alterar a rede Wi-Fi, basta reiniciar o sistema pressionando o botão correspondente, o que apagará as credenciais antigas e solicitará as novas informações de rede.

Para agendar a irrigação, é necessário inserir o horário (horas e minutos), a duração da irrigação (em minutos) e selecionar quais dos oito vasos serão irrigados. No horário programado, o sistema abrirá as válvulas especificadas e monitorará o fluxo de água. Se em algum momento durante a irrigação a água estiver ausente, o sistema ativará a bomba do reservatório para garantir o fornecimento contínuo de água, proporcionando segurança durante viagens, quando o registro de água estiver fechado.

Caso a bomba esteja em funcionamento e o sistema não detecte fluxo de água, ela será desligada automaticamente, e as válvulas serão fechadas como medida de precaução.

Além disso, o sistema oferece suporte ao acionamento imediato, dispensando a necessidade de agendamento prévio. Nesse modo, o sistema continua a utilizar a verificação de água para acionar a bomba conforme necessário, garantindo o fornecimento adequado de água para a irrigação. O sistema também é projetado para desligar a bomba automaticamente caso não seja detectada água no sistema, evitando danos ou desperdício de energia

## Comunicação

O sistema se comunica com o aplicativo móvel por meio do servidor MQTT, utilizando JSON para troca de informações. É possível configurar o servidor MQTT através do menuconfig.

As chaves JSON para comunicação são as seguintes:

    **"a"**: para o acionamento imediato das válvulas;
    **"v"**: para o agendamento dos vasos a serem irrigados;
    **"h"**: para especificar o horário do agendamento;
    **"t"**: para definir o tempo de irrigação.
