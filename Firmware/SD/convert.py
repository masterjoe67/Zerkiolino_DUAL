import struct
import tkinter as tk
from tkinter import filedialog, messagebox
from PIL import Image
import os

def convert_image():
    # 1. Selezione del file
    input_path = filedialog.askopenfilename(
        title="Seleziona l'immagine da convertire",
        filetypes=[("Image files", "*.jpg *.jpeg *.png *.bmp")]
    )
    
    if not input_path:
        return

    # 2. Setup output path
    output_path = os.path.splitext(input_path)[0] + ".raw"
    
    try:
        # 3. Elaborazione immagine
        img = Image.open(input_path).convert('RGB')
        # Risoluzione standard 640x480
        img = img.resize((640, 480), Image.Resampling.LANCZOS) 
        
        with open(output_path, 'wb') as f:
            for y in range(480):
                for x in range(640):
                    r, g, b = img.getpixel((x, y))
                    
                    # --- SCALATURA A RGB565 ---
                    # Rosso: da 8 bit (0-255) a 5 bit (0-31)
                    r5 = (r >> 3) & 0x1F
                    # Verde: da 8 bit (0-255) a 6 bit (0-63)
                    g6 = (g >> 2) & 0x3F
                    # Blu:   da 8 bit (0-255) a 5 bit (0-31)
                    b5 = (b >> 3) & 0x1F
                    
                    # --- IMPACCHETTAMENTO RGB565 ---
                    # Bit 15-11: RED
                    # Bit 10-5:  GREEN
                    # Bit 4-0:   BLUE
                    pixel_value = (r5 << 11) | (g6 << 5) | b5
                    
                    # --- SCRITTURA LITTLE ENDIAN ('<H') ---
                    # Esempio: Pixel = 0xABCD
                    # Byte sul file: 0xCD poi 0xAB
                    # Perfetto per il tuo NeoRV32 e PetitFS
                    f.write(struct.pack('<H', pixel_value))
        
        messagebox.showinfo("Successo!", f"File generato:\n{output_path}\n\nFormato: RGB565 (16-bit LE)")
        
    except Exception as e:
        messagebox.showerror("Errore", f"Si è verificato un problema:\n{str(e)}")

# --- Interfaccia Grafica ---
root = tk.Tk()
root.title("Tiny_DSO Image Converter (RGB565)")
root.geometry("400x200")

label = tk.Label(root, text="Convertitore Immagini per Tiny_DSO_VGA", pady=20, font=('Arial', 10, 'bold'))
label.pack()

btn = tk.Button(root, text="Scegli Immagine e Converti (RGB565)", command=convert_image, 
                bg="#0078D7", fg="white", padx=20, pady=10, font=('Arial', 9, 'bold'))
btn.pack(pady=10)

info = tk.Label(root, text="Output: 640x480 RAW (RGB565 - 16bit LE)", fg="gray")
info.pack()

root.mainloop()