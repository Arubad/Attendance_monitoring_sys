import tkinter as tk
import pandas as pd
from tkinter import messagebox
from matplotlib import pyplot as plt

def load_data():
    try:
        return pd.read_csv("attendance.csv")
    except FileNotFoundError:
        return pd.DataFrame(columns=["Roll No", "Date", "Present", "Timestamp"])

def calculate_summary():
    roll = roll_var.get().strip()
    if not roll:
        summary_text.set("Please enter a roll number.")
        return

    df = load_data()
    df = df[df["Roll No"].astype(str) == roll]
    if df.empty:
        summary_text.set(f"No records found for Roll No: {roll}")
        return

    present_df = df[df["Present"] == 1]
    absent_df = df[df["Present"] == 0]
    total_present = len(present_df)
    total_absent = len(absent_df)
    total = total_present + total_absent
    percentage = (total_present / total * 100) if total > 0 else 0

    # Display in GUI
    summary = (
        f"Summary for Roll No: {roll}\n\n"
        f"Present Days: {total_present}\n"
        f"Absent Days: {total_absent}\n"
        f"Attendance Percentage: {percentage:.2f}%\n\n"
        f"Timestamps Present:\n" +
        "\n".join(f"{row['Date']} at {row['Timestamp']}" for _, row in present_df.iterrows()) +
        "\n\nTimestamps Absent:\n" +
        "\n".join(f"{row['Date']}" for _, row in absent_df.iterrows())
    )
    summary_text.set(summary)

    # Plot pie chart
    plot_pie_chart(total_present, total_absent)

def plot_pie_chart(present, absent):
    fig, ax = plt.subplots()
    ax.pie([present, absent], labels=["Present", "Absent"], autopct='%1.1f%%', colors=["green", "red"])
    ax.set_title("Attendance Distribution")
    plt.show()

# GUI
root = tk.Tk()
root.title("Student Attendance Summary")

tk.Label(root, text="Enter Roll No:").pack(pady=(10, 0))
roll_var = tk.StringVar()
tk.Entry(root, textvariable=roll_var, width=20).pack(pady=5)

tk.Button(root, text="Get Summary", command=calculate_summary).pack(pady=5)

summary_text = tk.StringVar()
tk.Label(root, textvariable=summary_text, justify="left", font=("Courier", 10), anchor="w").pack(padx=10, pady=10)

root.mainloop()
