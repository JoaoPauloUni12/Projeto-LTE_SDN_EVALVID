#!/bin/bash
set -e

BASE_DIR="results"
VIDEOS=("harbour" "crew")
SCENARIOS=("0" "1")

mkdir -p "$BASE_DIR"

echo "=== Gerando traces EvalVid (.st) para harbour e crew ==="

# Y4M -> MP4 -> hinted -> .st
for base in "${VIDEOS[@]}"; do
  y4m="${base}_4cif.y4m"
  m4v="${base}.m4v"
  hinted="${base}_hint.mp4"
  st="st_${base}.st"

  if [ ! -f "$st" ]; then
    if [ ! -f "$y4m" ]; then
      echo "  !! Arquivo $y4m não encontrado, pulei."
      continue
    fi
    echo "  -> Convertendo $y4m"

    # 1) Y4M -> MP4 (H.264/AVC) - 704x576, 30 fps
    ffmpeg -y -i "$y4m" -c:v libx264 -preset slow -crf 22 -c:a copy "$m4v"

    # 2) Hint track com MP4Box (GPAC)
    MP4Box -hint -mtu 1024 -fps 30 -add "$m4v" "$hinted"

    # 3) MP4 hinted -> trace .st
    mp4trace -f -s 192.168.0.2 12346 "$hinted" > "$st"
  else
    echo "  -> $st já existe, pulando geração."
  fi
done

echo "=== Limpando resultados antigos ==="
rm -rf "$BASE_DIR"
mkdir -p "$BASE_DIR"

# remove resultados antigos locais (mas não fontes)
rm -f harbour_sd_ue*_s* harbour_rd_ue*_s* \
      crew_sd_ue*_s*    crew_rd_ue*_s* \
      recv_* *.264 *.yuv psnr_*.txt mos_*.txt s*_QoS_*.txt s*_QoS_*.xml 2>/dev/null || true

for s in "${SCENARIOS[@]}"; do
  echo "=== Rodando cenário $s ==="
  OUT_DIR="$BASE_DIR/scenario_$s"
  mkdir -p "$OUT_DIR"

  ./ns3 run "lte-sdn-evalvid --simTime=30 --scenario=$s"

  echo "  -> Organizando arquivos EvalVid..."
  set +e
  mv harbour_sd_ue*_s${s} "$OUT_DIR"/ 2>/dev/null
  mv harbour_rd_ue*_s${s} "$OUT_DIR"/ 2>/dev/null
  mv crew_sd_ue*_s${s}    "$OUT_DIR"/ 2>/dev/null
  mv crew_rd_ue*_s${s}    "$OUT_DIR"/ 2>/dev/null

  echo "  -> Organizando arquivos QoS..."
  mv s${s}_QoS_vazao.txt        "$OUT_DIR"/ 2>/dev/null
  mv s${s}_QoS_perda.txt        "$OUT_DIR"/ 2>/dev/null
  mv s${s}_QoS_jitter.txt       "$OUT_DIR"/ 2>/dev/null
  mv s${s}_QoS_delay.txt       "$OUT_DIR"/ 2>/dev/null  
  mv s${s}_QoS_flowmonitor.xml  "$OUT_DIR"/ 2>/dev/
  mv s${s}_QoS_drops_largura_ue*.txt "$OUT_DIR"/ 2>/dev/null
  set -e
done

echo "=== Reconstruindo vídeos e calculando PSNR/MOS (EvalVid) ==="

REF_YUV_HARBOUR="harbour_ref.yuv"
REF_YUV_CREW="crew_ref.yuv"

# Referências YUV a partir do Y4M original
if [ ! -f "$REF_YUV_HARBOUR" ]; then
  ffmpeg -y -i harbour_4cif.y4m "$REF_YUV_HARBOUR"
fi
if [ ! -f "$REF_YUV_CREW" ]; then
  ffmpeg -y -i crew_4cif.y4m "$REF_YUV_CREW"
fi

for s in "${SCENARIOS[@]}"; do
  OUT_DIR="$BASE_DIR/scenario_$s"
  cd "$OUT_DIR"

  # Para cada SD (harbour/crew, qualquer UE)
  for sd in *sd_ue*_s${s}; do
    [ -e "$sd" ] || continue

    rd="${sd/_sd_/_rd_}"
    [ -f "$rd" ] || continue

    # sd: harbour_sd_ue0_s0 -> base=harbour, ue=0
    base="${sd%%_*}"              # harbour ou crew
    tmp="${sd#*_sd_ue}"           # 0_s0
    ue="${tmp%%_s*}"              # 0

    recv_base="${base}_ue${ue}_s${s}_recv"
    recv_mp4="${recv_base}.mp4"
    recv_yuv="${recv_base}.yuv"
    psnr_file="psnr_${base}_ue${ue}_s${s}.txt"
    mos_file="mos_${base}_ue${ue}_s${s}.txt"

    if [ "$base" = "harbour" ]; then
      st_file="../../st_harbour.st"
      hint_mp4="../../harbour_hint.mp4"
      ref_yuv="../../$REF_YUV_HARBOUR"
      width=704
      height=576
    else
      st_file="../../st_crew.st"
      hint_mp4="../../crew_hint.mp4"
      ref_yuv="../../$REF_YUV_CREW"
      width=704
      height=576
    fi

    echo "  -> Cenário $s, vídeo $base, UE $ue"

    # Reconstroi o vídeo recebido
    etmp4 -f -0 "$sd" "$rd" "$st_file" "$hint_mp4" "$recv_base"

    # Converte para YUV
    ffmpeg -y -i "$recv_mp4" "$recv_yuv"

    # PSNR
    psnr "$width" "$height" 420 "$recv_yuv" "$ref_yuv" > "$psnr_file"

    # MOS
    mos "$psnr_file" > "$mos_file"
  done

  cd - >/dev/null
done

echo "=== Tudo pronto. Resultados em $BASE_DIR/scenario_0 e _1 ==="

echo "=== Ativando venv e gerando gráficos (Python) ==="
source .venv/bin/activate
python3 post_process.py
deactivate