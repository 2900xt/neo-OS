package Utils;

import java.awt.Color;
import java.awt.image.*;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.time.Duration;
import java.time.LocalDateTime;

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
        LocalDateTime start = LocalDateTime.now();
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


        long count = width * height;
        double progress = 0;
        int progStrLen = 0;
        int bytes = width * height * 4  + 8;

        ByteBuffer byteBuf = ByteBuffer.allocate(bytes);
        byteBuf.put(intToByte(width));
        byteBuf.put(intToByte(height));

        for(int i = 0; i < height; i++)
        {
            for(int j = 0; j < width; j++)
            {
                Color c = new Color(image.getRGB(j, i));
                int rgb = ((int)c.getAlpha() << 24) | ((int)c.getRed() << 16) | ((int)c.getGreen() << 8) | (int)c.getBlue();
                byteBuf.put(intToByte(rgb));
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

        fileOut.write(byteBuf.array());

        System.out.println("\nTotal File Size: " + Math.round(bytes / 10000.0) / 100.0 + "M");
        Duration elapsedTime = Duration.between(start, LocalDateTime.now());
        System.out.println("Time: " + elapsedTime.toMillis() / 1000.0 + "s");

        double mbPerSecond = (Math.round(bytes / 10000.0) / 100.0) / (elapsedTime.toMillis() / 1000.0);
        System.out.println("Speed: " + Math.round(mbPerSecond * 10) / 10 + " MB/S");

        fileOut.close();
    }
}