# rcomp.x

rcomp.x: compressor em plugin VST3 (JUCE 8).
Controles: Threshold, Ratio, Attack, Release, Knee, Makeup e Mix.
Visual 8 bits: cores chapadas, sem gradientes, com uma janela de VU onde a onda de entrada entra pela direita, mais barras de VU e GR.

Compressor DSP original, com controles de threshold, ratio, attack, release, knee, makeup e mix.

## Compilar pelo GitHub Actions

1. Crie um repositório vazio no GitHub (ex.: `rcomp.x`).
2. No PC, dentro desta pasta:
   ```
   git add .
   git commit -m "rcomp.x: primeira versão"
   git branch -M main
   git remote add origin https://github.com/SEU_USUARIO/rcomp.x.git
   git push -u origin main
   ```
3. No GitHub, abra a aba **Actions** e espere o job **Build VST3** ficar verde.
4. Entre no run concluído e baixe o artefato **RCompX-VST3** (um `.vst3` dentro de um zip).
5. Copie a pasta `rcomp.x.vst3` para `C:\Program Files\Common Files\VST3\` e rescaneie os plugins no FL Studio.

Também dá para disparar manualmente em **Actions > Build VST3 > Run workflow**.

## Compilar localmente (Windows, Visual Studio 2022 com C++ desktop)

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target RCompX_VST3
```

## Estrutura

- `Source/RCompEngine.h`: DSP do compressor (curva com knee, detector com attack/release, makeup)
- `Source/PluginProcessor.*`: parâmetros (APVTS), processamento estéreo linkado, estado
- `Source/PluginEditor.*`: skin 8 bits (cores chapadas, knobs em pixel), janela de VU com onda
- `.github/workflows/build.yml`: build do VST3 no Windows
