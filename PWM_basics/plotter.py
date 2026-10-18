import serial 
import matplotlib.pyplot as plt
import numpy as np 
import time

# Hardware parameters [hardcoded]
Vdd = 5.0 # V
res = 1023 # ADC resolution
# Array Size
ARRAY_SIZE = 1024
# STOP FEATURE
is_paused = False


def toggle_pause(event):
    "Pause Functionality function"

    global is_paused
    if event.key == ' ': #trigger with the spacebar
        is_paused = not is_paused
        if is_paused:
            print("Paused - Press spacebar to resume")
        else:
            print ("RESUMED")

def connect_and_handshake(comport, baudrate):
    "Connect to Arduino and extract data"

    print("Connecting to Arduino...")
     # Declare the serial port opbject. It allows communicating with the USB
    # baudrate must match netween MCU and Python
    # Timeout: it fix the time Python must wait data arriving from the USB port
    # If the port stop receiving data, the this timeout prevents freezing
    ser = serial.Serial(comport, baudrate, timeout=0.1)
    time.sleep(2) 

    print("Waiting Arduino configuration...")
    Fs = 0 # Error if not received
    while True:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line.startswith("CONNECTION:"):
                period_us = int(line.split(":")[1])
                Fs = 1e6 / period_us
                print(f"Sampling frequency set to: {Fs} Hz")
                break
    return ser, Fs

def declare_plots(Fs):
    "Declare the plot objects"

    plt.ion()
    # --- Window 1: DC Signal ---
    fig1 = plt.figure()
    fig1.canvas.manager.set_window_title('DC Signal Plot')
    ax1 = fig1.add_subplot(111)
    ax1.set_ylim([-5, 5])
    ax1.set_xlim([0, ARRAY_SIZE * (1/Fs)])
    line_dc, = ax1.plot(np.zeros(ARRAY_SIZE), label="DC", color='blue', marker='.')
    ax1.legend(loc='upper right')
    ax1.set_xlabel('Time (seconds)')
    ax1.set_ylabel('Voltage (V)')
    ax1.set_title("DC Signal")
    ax1.grid(True, linestyle='--', alpha=0.7) 

    # --- Window 2: Full Signal ---
    fig2 = plt.figure()
    fig2.canvas.manager.set_window_title('Full Signal Plot')
    ax2 = fig2.add_subplot(111)
    ax2.set_ylim([-5, 5])
    ax2.set_xlim([0, ARRAY_SIZE * (1/Fs)])
    line, = ax2.plot(np.zeros(ARRAY_SIZE), label="full", color='orange', marker='.')
    ax2.legend(loc='upper right')
    ax2.set_xlabel('Time (seconds)')
    ax2.set_ylabel('Voltage (V)')
    ax2.set_title("Full Signal")
    ax2.grid(True, linestyle='--', alpha=0.7)


    # --- Window 3: FFT of Full Signal ---
    fig3 = plt.figure()
    fig3.canvas.manager.set_window_title("FFT Plot")
    ax3 = fig3.add_subplot(111)
    # Generate X-axis frequency bins
    freqs = np.fft.rfftfreq(ARRAY_SIZE, 1.0/Fs)
    # Empty FFT vector
    fft_data = np.zeros(len(freqs))
    line_fft, = ax3.plot(freqs, fft_data, label="FFT", color='red', marker='.')
    ax3.set_xlabel('Frequency (Hz)')
    ax3.set_ylabel('Magnitude')
    ax3.set_title("FFT of Full Signal")
    ax3.grid(True, linestyle='--', alpha=0.7)


    # Connect the spacebar listener to all three of your windows
    fig1.canvas.mpl_connect('key_press_event', toggle_pause)
    fig2.canvas.mpl_connect('key_press_event', toggle_pause)
    fig3.canvas.mpl_connect('key_press_event', toggle_pause)

    # Return fig, lines and axis as objects for the other functions
    return {
        "figs": (fig1, fig2, fig3),
        "lines": (line_dc, line, line_fft),
        "ax_fft": ax3
    }

def signal_analysis(ser, Fs):
    "Analyse input signals from Arduino serial port"
    
    # Declare data as global so we can modify the array
    global data_dc, data, is_paused
    # Data array initialization
    data_dc = np.zeros((ARRAY_SIZE))
    data = np.zeros((ARRAY_SIZE))

    # Unpack the plot objects
    plot_objects = declare_plots(Fs)
    fig1, fig2, fig3 = plot_objects["figs"]
    line_dc, line, line_fft = plot_objects["lines"]
    ax3 = plot_objects["ax_fft"]

    while True: 
        batch_received = False
        # Batch Reading: empty the whole buffer before plottig
        while ser.in_waiting> 0: 
            raw_string = ser.readline().decode('utf-8', errors='ignore').strip()
            if raw_string:
                try:
                    #split the incoming data
                    values = raw_string.split()

                    #check data length
                    if len(values) == 2:
                        #convert signals: string -> float
                        dc = float(values[0]) * (Vdd/res)
                        full = float(values[1]) * (Vdd/res)
                        #append new data to the arrays
                        data_dc = np.append(data_dc, dc)
                        data = np.append(data, full) 
                        batch_received = True
                except ValueError:
                    pass
        # Plotting
        if batch_received:
            if len(data)> ARRAY_SIZE:
                data_dc = data_dc[-ARRAY_SIZE:]
                data = data [-ARRAY_SIZE:]

            if not is_paused:
                #true time axis creation
                time_axis = np.arange(len(data))*(1/Fs)

                # update time lines 
                line_dc.set_ydata(data_dc)
                line_dc.set_xdata(time_axis)

                line.set_ydata(data)
                line.set_xdata(time_axis)

                # Calculate the FFT 
                windowed_data = data * np.hanning(len(data))
                fft_data = np.abs(np.fft.rfft(windowed_data, n = ARRAY_SIZE)) 
                line_fft.set_ydata(fft_data)
                # Dynamically stretch the FFT Y-axis to fit changing peak heights
                ax3.relim()
                ax3.autoscale_view(scalex=False, scaley=True)

                # Draw all three windows
                fig1.canvas.draw()
                fig2.canvas.draw()
                fig3.canvas.draw()
                
            fig1.canvas.flush_events()
            fig2.canvas.flush_events()
            fig3.canvas.flush_events()
        
        # Prevent the while loop from maxing out your CPU when no data is arriving
        time.sleep(0.005)

if __name__ == "__main__":
    active_serial, detected_Fs = connect_and_handshake('COM5', 115200)
    signal_analysis(active_serial, detected_Fs)
