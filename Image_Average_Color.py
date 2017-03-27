from colorthief import ColorThief
import sys
import serial
import time

# Opens port to Arduino
ser = serial.Serial('COM3', 9600, timeout=0)

# Average color of picture
def average_color(filename):
    
    # Top colors for the color range of the dominant pallete
    color_range = 10
    
    # Boolean value, used so that only the first accurate average value is printed
    correct_color_found = 0;

    # Color theif is the function that finds the average values of the picture
    color_thief = ColorThief(filename)

    # Finds the top *color_range* of colors that dominate the image
    dominant_palette = color_thief.get_palette(color_count=color_range, quality=10)
    
    # Checks the top dominate colors and outputs the first color whose dominance
    # is not a shade of white
    for i in range(0, color_range - 1):
        if (dominant_palette[i][0] < 210) | (dominant_palette[i][1] < 210) | (dominant_palette[i][2] < 210):
            # If this color is the first non-white one then it is output
            if(correct_color_found == 0):
                # Prints the r, g, b values of the dominant color in the image
                print(dominant_palette[i])

                # Converts the r, g, b values to bytes for transfering to the Arduino
                bytes(dominant_palette[i][0])
                bytes(dominant_palette[i][1])
                bytes(dominant_palette[i][2])

                # Transfers the bytes to the Arduino
                ser.write(dominant_palette[i][0])
                ser.write(dominant_palette[i][1])
                ser.write(dominant_palette[i][2])

                # Changes correct_color_found to true so another color won't be printed
                correct_color_found = 1;

if __name__ == '__main__':

    # filename and sys.argv are hardcoded placeholders when using IDLE
    filename = "apple.jpg"
    sys.argv = [sys.argv[0], filename]

    # If the filename is given then find the average color
    if len(sys.argv) > 1:
        average_color(sys.argv[1])
    
    
