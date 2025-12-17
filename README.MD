# LTE + SDN (OFSwitch13) + EvalVid no ns-3

Repositório do trabalho prático da disciplina **Gerência de Redes de Computadores** (FACSI/UNIFESSPA), avaliando o impacto da priorização SDN na qualidade de transmissão de vídeo em rede LTE usando **ns-3.39 + LENA + OFSwitch13 + EvalVid**.

## 1. Estrutura do repositório

- `lte-sdn-evalvid.cc`  
  Script principal de simulação (colocar em `scratch/` do ns-3).

- `run_tests.sh`  
  Automatiza execução da simulação para os dois cenários e processamento EvalVid (reconstrução dos vídeos, PSNR, MOS, etc.).

- `post_process.py`  
  Script Python que lê os arquivos de QoS/QoE gerados e produz os gráficos (`.jpg`).

- `st_harbour.st`, `st_crew.st`  
  Traces EvalVid dos vídeos **harbour** e **crew**.

- `harbour_hint.mp4`, `crew_hint.mp4`  
  Versões “hinted” dos vídeos para uso com o EvalVid.

- `results/scenario_0/` e `results/scenario_1/`  
  Arquivos de saída das simulações:
  - QoS: `s*_QoS_vazao.txt`, `s*_QoS_jitter.txt`, `s*_QoS_delay.txt`, `s*_QoS_perda.txt`,
    `s*_QoS_drops_largura_ue*.txt`
  - QoE: `psnr_*.txt`, `mos_*.txt`
  - Gráficos: `qos_*.jpg`, `psnr_s*.jpg`, `largura_ue_s*.jpg`

- `3-trabalho-pratico-gerencia-2025.4.pdf`  
  Enunciado oficial do trabalho.

## 2. Requisitos

- ns-3.39 compilado com CMake
- Módulo **OFSwitch13** integrado ao ns-3
- Ferramentas EvalVid instaladas (`mp4trace`, `etmp4`, `psnr`, `mos`)
- Python 3.x com:
  - `matplotlib`
  - `numpy`

## 3. Como reproduzir a simulação

1. Copie `lte-sdn-evalvid.cc` para a pasta `scratch/` do seu ns-3:
cp trabalho-gerencia/lte-sdn-evalvid.cc /caminho/para/ns-3-dev/scratch/


2. Na raiz do ns-3:
./ns3 build


3. Execute o script de testes (gera cenários 0 e 1 + EvalVid):
./run_tests.sh


Após a execução, os resultados estarão em `results/scenario_0` e `results/scenario_1`.

## 4. Processamento e geração de gráficos
ja gerado automaticamente pelo run_tests.sh


Ele gera, nas pastas de cenário:

- `qos_throughput.jpg` – vazão média por cenário  
- `qos_jitter.jpg` – jitter médio por cenário  
- `qos_delay.jpg` – delay médio por cenário  
- `qos_loss.jpg` – perda de pacotes por cenário  
- `psnr_s0.jpg`, `psnr_s1.jpg` – PSNR médio por UE  
- `largura_ue_s0.jpg`, `largura_ue_s1.jpg` – throughput total por UE

## 5. Descrição dos cenários

- **Cenário 0 – Sem SDN/priorização**
  - Scheduler LTE: Round Robin
  - Switch OFSwitch13 sem regra OpenFlow adicional
  - Apenas bearer default para os UEs

- **Cenário 1 – Com SDN + QoS**
  - Scheduler LTE: Proportional Fair
  - Regra OpenFlow de alta prioridade para tráfego UDP destino porta 8000 (vídeo)
  - Bearer dedicado `GBR_CONV_VIDEO` para dois UEs VIP

## 6. Métricas avaliadas

- **QoS**
  - Vazão (throughput)
  - Jitter
  - Atraso médio (delay)
  - Perda de pacotes

- **QoE**
  - PSNR médio por UE
  - MOS (Mean Opinion Score) por fluxo

## 7. Referências principais

- Documentação ns-3: https://www.nsnam.org/docs/release/3.39  
- Módulo LTE: https://www.nsnam.org/docs/models/html/lte.html  
- EvalVid: http://www2.tkn.tu-berlin.de/research/evalvid/  
- OFSwitch13 para ns-3: https://github.com/oftestbed/ofswitch13-ns3  
