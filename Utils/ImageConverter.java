package Utils;

import java.awt.Color;
import java.awt.image.*;
import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;

import javax.swing.ImageIcon;

public class ImageConverter
{
    public static void usage()
    {
        System.out.println("Usage: \nImageConverter.java <input> <output.nic>\n\nNEO-OS Utility to convert binary image files to raw .nic files");
        System.exit(1);
    }
    public static void main(String []args) throws IOException
    {
        if(args.length != 2)
        {
            usage();
        }

        ImageIcon input = new ImageIcon(args[0]);
        BufferedImage image = new BufferedImage(input.getIconWidth(), input.getIconHeight(), BufferedImage.TYPE_INT_RGB);
        image.getGraphics().drawImage(input.getImage(), 0, 0, input.getIconWidth(), input.getIconHeight(), null);

        BufferedWriter fileOut = new BufferedWriter(new FileWriter(args[1]));
        fileOut.append(image.getWidth() + "\0");
        fileOut.append(image.getHeight() + "\0");

        for(int i = 0; i < image.getHeight(); i++)
        {
            for(int j = 0; j < image.getWidth(); j++)
            {
                Color c = new Color(image.getRGB(j, i));
                int rgb = ((int)c.getRed() << 16) | ((int)c.getGreen() << 8) | (int)c.getBlue();
                fileOut.append(rgb + "\0");
            }
        }

        fileOut.close();
    }
}