import pandas as pd
import matplotlib.pyplot as plt

# Lê o arquivo CSV com proteção contra caracteres ocultos
df = pd.read_csv("log.csv", encoding='utf-8-sig')

# Define tempo como número da amostra
tempo = df['numero_amostra']

# ---------- GRÁFICO COMBINADO: Aceleração ----------
plt.figure()
plt.plot(tempo, df['accel_x'], label='Ax')
plt.plot(tempo, df['accel_y'], label='Ay')
plt.plot(tempo, df['accel_z'], label='Az')
plt.xlabel("Tempo (amostras)")
plt.ylabel("Aceleração (g)")
plt.title("Aceleração - Eixos X, Y, Z")
plt.legend()
plt.grid(True)

# ---------- GRÁFICO COMBINADO: Giroscópio ----------
plt.figure()
plt.plot(tempo, df['giro_x'], label='Gx')
plt.plot(tempo, df['giro_y'], label='Gy')
plt.plot(tempo, df['giro_z'], label='Gz')
plt.xlabel("Tempo (amostras)")
plt.ylabel("Velocidade Angular (°/s)")
plt.title("Giroscópio - Eixos X, Y, Z")
plt.legend()
plt.grid(True)

# ---------- GRÁFICOS INDIVIDUAIS DE ACELERAÇÃO ----------
plt.figure()
plt.plot(tempo, df['accel_x'])
plt.xlabel("Tempo (amostras)")
plt.ylabel("Ax (g)")
plt.title("Aceleração - Eixo X")
plt.grid(True)

plt.figure()
plt.plot(tempo, df['accel_y'])
plt.xlabel("Tempo (amostras)")
plt.ylabel("Ay (g)")
plt.title("Aceleração - Eixo Y")
plt.grid(True)

plt.figure()
plt.plot(tempo, df['accel_z'])
plt.xlabel("Tempo (amostras)")
plt.ylabel("Az (g)")
plt.title("Aceleração - Eixo Z")
plt.grid(True)

# ---------- GRÁFICOS INDIVIDUAIS DE GIROSCÓPIO ----------
plt.figure()
plt.plot(tempo, df['giro_x'])
plt.xlabel("Tempo (amostras)")
plt.ylabel("Gx (°/s)")
plt.title("Giroscópio - Eixo X")
plt.grid(True)

plt.figure()
plt.plot(tempo, df['giro_y'])
plt.xlabel("Tempo (amostras)")
plt.ylabel("Gy (°/s)")
plt.title("Giroscópio - Eixo Y")
plt.grid(True)

plt.figure()
plt.plot(tempo, df['giro_z'])
plt.xlabel("Tempo (amostras)")
plt.ylabel("Gz (°/s)")
plt.title("Giroscópio - Eixo Z")
plt.grid(True)

plt.show()
