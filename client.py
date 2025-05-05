import socket
from tkinter import *

def submit_attendance():
    roll = entry_roll.get().strip()
    if not roll:
        response.config(text="Roll Number is required.")
        return

    try:
        s = socket.socket()
        s.connect(('127.0.0.1', 9090))
        s.send(roll.encode())
        msg = s.recv(1024).decode()
        response.config(text=msg)
        s.close()
    except Exception as e:
        response.config(text=f"Connection failed: {e}")

root = Tk()
root.title("Attendance Client")

Label(root, text="Roll Number:").pack()
entry_roll = Entry(root)
entry_roll.pack()

Button(root, text="Submit", command=submit_attendance).pack(pady=5)
response = Label(root, text="")
response.pack(pady=5)

root.mainloop()
