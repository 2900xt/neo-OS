import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.filechooser.FileFilter;

import java.awt.Graphics;
import java.awt.image.BufferedImage;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;

public class ImageViewer extends JPanel
{
    private BufferedImage img;
    public ImageViewer(BufferedImage img)
    {
        this.img = img;
    }

    public void paintComponent(Graphics g)
    {
        g.drawImage(img, 0, 0, getWidth(), getHeight(), null);
    }

    public static final byte[] signature = "NIC-1.0".getBytes();

    public static int endianConvert(int bigEndian)
    {
        int b1 = (bigEndian & 0xFF) << 24;
        int b2 = (bigEndian & 0xFF00) << 8;
        int b3 = (bigEndian & 0xFF0000) >>> 8;
        int b4 = (bigEndian >>> 24) & 0xFF;
        return b1 | b2 | b3 | b4;
    }

    public static void main(String []args) throws IOException
    {
        JFileChooser fileChooser = new JFileChooser(".");
        fileChooser.setFileFilter(new FileFilter() {

            @Override
            public boolean accept(File arg0) {
                return arg0.getName().endsWith(".nic") || arg0.isDirectory();
            }

            @Override
            public String getDescription() {
                return ".nic";
            }
            
        });

        fileChooser.showOpenDialog(null);
        File imageFile = fileChooser.getSelectedFile();

        if(imageFile == null)
        {
            System.err.println("No File Selected!");
            System.exit(0);
        }

        DataInputStream fileIn = new DataInputStream(new FileInputStream(imageFile));

        byte[] img_sig = new byte[signature.length];
        fileIn.read(img_sig);

        for(int i = 0; i < signature.length; i++)
        {
            if(img_sig[i] != signature[i])
            {
                System.err.println("Invalid File Signature!");
                System.exit(1);
            }
        }

        int width = endianConvert(fileIn.readInt());
        int height = endianConvert(fileIn.readInt());

        BufferedImage image = new BufferedImage(width, height, BufferedImage.TYPE_INT_RGB);
        for(int y = 0; y < height; y++)
        {
            for(int x = 0; x < width; x++)
            {
                image.setRGB(x, y, endianConvert(fileIn.readInt()));
            }
        }

        fileIn.close();

        JFrame frame = new JFrame("NIC Viewer");
        frame.setSize(width, height);
        frame.setLocation(0, 0);
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setContentPane(new ImageViewer(image));
        frame.setVisible(true);
    }
}