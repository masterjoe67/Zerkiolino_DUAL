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
        # La risoluzione del tuo Tiny_DSO
        img = img.resize((640, 480)) 
        
        with open(output_path, 'wb') as f:
            for y in range(480):
                for x in range(640):
                    r, g, b = img.getpixel((x, y))
                    
                    # Scala da 8-bit a 3-bit (0-255 -> 0-7)
                    r3 = (r >> 5) & 0x07
                    g3 = (g >> 5) & 0x07
                    b3 = (b >> 5) & 0x07
                    
                    # IMPACCHETTAMENTO BGR333 (Secondo le tue #define)
                    # bit 8-6: BLUE
                    # bit 5-3: GREEN
                    # bit 2-0: RED
                    pixel_value = (b3 << 6) | (g3 << 3) | r3
                    
                    # SCRITTURA LITTLE ENDIAN ('<H')
                    # Byte 1: GGG RRR (bit bassi)
                    # Byte 2: 0000000 B (bit alti)
                    f.write(struct.pack('<H', pixel_value))
        
        messagebox.showinfo("Successo!", f"File generato:\n{output_path}\n\nFormato: BGR333 (16-bit word)")
        
    except Exception as e:
        messagebox.showerror("Errore", f"Si è verificato un problema:\n{str(e)}")

# --- Interfaccia Grafica ---
root = tk.Tk()
root.title("Tiny_DSO Image Converter")
root.geometry("400x200")

label = tk.Label(root, text="Convertitore Immagini per VGA DE0-NANO", pady=20, font=('Arial', 10, 'bold'))
label.pack()

btn = tk.Button(root, text="Scegli Immagine e Converti", command=convert_image, 
                bg="#4CAF50", fg="white", padx=20, pady=10, font=('Arial', 9, 'bold'))
btn.pack(pady=10)

info = tk.Label(root, text="Output: 640x480 RAW (BGR333 - 16bit LE)", fg="gray")
info.pack()

root.mainloop()