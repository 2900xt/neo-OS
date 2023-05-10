package Utils;

import java.awt.Color;
import java.awt.image.*;
import java.io.BufferedWriter;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;

import javax.swing.ImageIcon;

public class ImageConverter
{
    public static void usage()
    {
        System.out.println("Usage: \nImageConverter.java <input> <output.nic> <width (optional) > <height (optional) >\n\nNEO-OS Utility to convert binary image files to raw .nic files");
        System.exit(1);
    }

    public static byte[] intToByte(int i)
    {
        return new byte[]{(byte)i , (byte)(i >> 8) , (byte)(i >> 16) , (byte)(i >> 24)};
    }

    public static void main(String []args) throws IOException
    {
        if(args.length != 4 && args.length != 2)
        {
            usage();
        }

        int width, height;

        ImageIcon input = new ImageIcon(args[0]);

        if(args.length == 4)
        {
            width = Integer.parseInt(args[2]);
            height = Integer.parseInt(args[3]);
        } else 
        {
            width = input.getIconWidth();
            height = input.getIconHeight();
        }

        BufferedImage image = new BufferedImage(width, height, BufferedImage.TYPE_INT_RGB);
        image.getGraphics().drawImage(input.getImage(), 0, 0, width, height, null);


        FileOutputStream fileOut = new FileOutputStream(args[1]);
        fileOut.write(intToByte(width));
        fileOut.write(intToByte(height));

        long count = width * height;
        double progress = 0;
        int progStrLen = 0;

        for(int i = 0; i < height; i++)
        {
            for(int j = 0; j < width; j++)
            {
                Color c = new Color(image.getRGB(j, i));
                int rgb = ((int)c.getAlpha() << 24) | ((int)c.getRed() << 16) | ((int)c.getGreen() << 8) | (int)c.getBlue();
                fileOut.write(intToByte(rgb));
            }

            progress = (i * width) / (double)count;
            progress = Math.round(progress * 1000) / 10.0;
            String progString = "Progress: " + progress + "%";

            for(int k = 0; k < progStrLen; k++)
            {
                System.out.print('\b');
            }

            progStrLen = progString.length();
            System.out.print(progString);
        }

        double bytes = width * height * 4  + 8;
        System.out.println("\nTotal File Size: " + Math.round(bytes / 10000) / 100.0 + "M");

        fileOut.close();
    }
}