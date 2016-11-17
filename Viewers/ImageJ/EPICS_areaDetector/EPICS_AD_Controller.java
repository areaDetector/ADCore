// EPICS_AD_Controller.java
// Original authors
//      Tim Madden, APS
//      Mark Rivers, University of Chicago
//      Chris Roehrig, APS

import ij.*;
import ij.process.*;
import java.awt.*;
import ij.plugin.*;
import ij.gui.ImageCanvas;
import ij.gui.ImageWindow;
import java.io.*;
import java.text.*;
import java.awt.event.*;
import java.util.*;
import javax.swing.*;
import javax.swing.border.*;

import gov.aps.jca.*;
import gov.aps.jca.dbr.*;
import gov.aps.jca.configuration.*;
import gov.aps.jca.event.*;

public class EPICS_AD_Viewer implements PlugIn, MouseListener {
    
    private static final short UPPER_LEFT = 1;
    private static final short NONE = 0;
    private static final short ROTATE_90 = 1;
    private static final short ROTATE_180 = 2;
    private static final short ROTATE_270 = 3;
    private static final short MIRROR= 4;
    private static final short ROTATE_90_MIRROR = 5;
    private static final short ROTATE_180_MIRROR = 6;
    private static final short ROTATE_270_MIRROR = 7;

    ImagePlus img;

    int imageSizeX = 0;
    int imageSizeY = 0;
    int imageSizeZ = 0;

    FileOutputStream debugFile;
    PrintStream debugPrintStream;
    Properties properties = new Properties();
    String propertyFile = "EPICS_AD_Controller.properties";

    JCALibrary jca;
    DefaultConfiguration conf;
    Context ctxt;

    /* These are EPICS channel objects to write the ROI
     * back to the areaDetector software.
     */
    Channel ch_minCamX;           //This is the start of the CCD readout in X
    Channel ch_minCamY;           //This is the start of the CCD readout in Y
    Channel ch_minCamX_RBV;       //This is the readback value of X of the CCD readout
    Channel ch_minCamY_RBV;       //This is the readback value of Y of the CCD readout
    Channel ch_sizeCamX;          //This is the size of the CCD readout in the X dimension
    Channel ch_sizeCamY;          //This is the size of the CCD readout in the Y dimension
    Channel ch_sizeCamArrayX_RBV; //This is the size of the image array of the CCD in X
    Channel ch_sizeCamArrayY_RBV; //This is the size of the image array of the CCD in Y
    Channel ch_camBinX_RBV;       //This is the amount of binning for the CCD readout in X
    Channel ch_camBinY_RBV;       //This is the amount of binning for the CCD readout in Y
    Channel ch_minRoiX;           //This is the start of the ROI in X
    Channel ch_minRoiY;           //This is the start of the ROI in Y
    Channel ch_minRoiX_RBV;       //This is the readback value of X in the ROI
    Channel ch_minRoiY_RBV;       //This is the readback value of Y in the ROI
    Channel ch_sizeRoiX;          //This is the size of the ROI in the X dimension
    Channel ch_sizeRoiY;          //This is the size of the ROI in the Y dimension
    Channel ch_sizeRoiArrayX_RBV; //This is the size of the image array for the ROI in X
    Channel ch_sizeRoiArrayY_RBV; //This is the size of the image array for the ROI in Y
    Channel ch_roiBinX_RBV;       //This is the amount of binning for the ROI in X
    Channel ch_roiBinY_RBV;       //This is the amount of binning for the ROI in Y
    Channel ch_maxSizeCamX;       //This is the maximum dimension of X
    Channel ch_maxSizeCamY;       //This is the maximum dimension of Y
    Channel ch_transType;         //This represents the way the image is flipped or rotated.

    JFrame frame;

    JTextField StatusText;
    JTextField cameraPrefixText;
    JTextField transformPrefixText;
    JTextField roiPrefixText;
    JTextField tweakAmountText;
    JButton resetRoiButton;
    JRadioButton setNoneRadioButton;
    JRadioButton setCCDReadoutRadioButton;
    JRadioButton setROIRadioButton;
    JRadioButton preTransformRadioButton;
    JRadioButton postTransformRadioButton;
    JCheckBox useTransformCheckBox;

    boolean isDebugMessages;
    boolean isDebugFile;
    boolean isPluginRunning;
    boolean isTransformConnected;
    boolean isCameraConnected;
    boolean isRoiConnected;
    boolean setCCD_Readout;
    boolean setROI;
    boolean useTransformPlugin;
    boolean isPreTransform;
    boolean isPostTransform;

    javax.swing.Timer timer;

    int cameraOriginX;
    int cameraOriginY;
    int cameraSizeX;
    int cameraSizeY;
    int cameraArraySizeX;
    int cameraArraySizeY;
    int cameraBinX;
    int cameraBinY;
    int roiOriginX;
    int roiOriginY;
    int roiSizeX;
    int roiSizeY;
    int roiArraySizeX;
    int roiArraySizeY;
    int roiBinX;
    int roiBinY;
    int tweakAmountPixels;
    String cameraPrefix;
    String transformPrefix;
    String roiPrefix;
    ImageWindow win;
    ImageCanvas canvas;

    /**
     * This method is called by ImageJ when the user starts the from the menu.
     * 
     * @param arg    This is a string argument that can be used to pass in
     *               information.
     */
    @Override
    public void run(String arg) {
        IJ.showStatus("epics running");
        try {
            isDebugFile = false;
            isDebugMessages = true;
            isPluginRunning = true;
            setCCD_Readout = false;
            setROI = false;
            useTransformPlugin = false;
            isPreTransform = false;
            isPostTransform = false;
            Date date = new Date();
            prevTime = date.getTime();
            cameraPrefix = "2dev:SIM1:cam1:";
            transformPrefix = "2dev:Trans1:";
            roiPrefix = "2dev:ROI1:";
            tweakAmountPixels = 1;
            readProperties();

            if (isDebugFile) {
                debugFile = new FileOutputStream(System.getProperty("user.home")
                        + System.getProperty("file.separator") + "IJEPICS_debug.txt");
                debugPrintStream = new PrintStream(debugFile);
            }
            javax.swing.SwingUtilities.invokeLater(
                    new Runnable() {
                        @Override
                        public void run() {
                            createAndShowGUI();
                        }
                    }
            );

            img = new ImagePlus(imagePrefix, new ByteProcessor(100, 100));
            img.show();
            img.close();

            startEPICSCA();
            // Connect to PVs from the camera.
            connectCameraPVs();
            // Connect to PVs from the transform plugin.
            // This do not need to succeed.
            connectTransformPVs();
            // Connect to PVs from the ROI plugin.
            // This does not need to succeed.
            connectRoiPVs();

            /* This simply polls for new data and updates the image if new data
             * is found.
             */
            while (isPluginRunning) {
                synchronized (this) {
                    wait(1000);
                }
            }

            if (isDebugFile) {
                debugPrintStream.close();
                debugFile.close();
                logMessage("Closed debug file", true, true);
            }

            timer.stop();
            writeProperties();
            disconnectCameraPVs();
            disconnectTransformPVs();
            disconnectRoiPVs();
            closeEPICSCA();
            IJ.showStatus("Exiting Server");

        } catch (Exception e) {
            IJ.log("Got exception: " + e.getMessage());
            e.printStackTrace();
            IJ.log("Close window, and reopen, try again");

            IJ.showStatus(e.toString());

            try {
                if (isDebugFile) {
                    debugPrintStream.close();
                    debugFile.close();
                }
            } catch (IOException ee) {
            }
        }
    }
    
    /**
     * This method overrides the standard mouseReleased method for the image
     * window.  When the use releases the mouse, it gets the rectangle drawn
     * by the user and uses that to write an ROI to either the camera or the
     * ROI plugin.  If no rectangle is drawn, the values retrieved are 0,0 and
     * the full dimensions in X and Y.
     * 
     * @param e     An event object.  It is not actually used.
     */
    public void setROI() {
        Rectangle imageRoi;

        int tempRoiX;
        int tempRoiY;
        int tempRoiWidth;
        int tempRoiHeight;
        int tempMaxSizeX;
        int tempMaxSizeY;

        if ((setCCD_Readout || (setROI && isRoiConnected)) && isCameraConnected) {
            try {
                // Get the position of the top right corner of the drawn ROI, as
                // well as the height and width.
                imageRoi = this.img.getProcessor().getRoi();
                tempRoiX = (int) imageRoi.getX();
                tempRoiY = (int) imageRoi.getY();
                tempRoiWidth = (int) imageRoi.getWidth();
                tempRoiHeight = (int) imageRoi.getHeight();

                // If the user is using the ROI plugin, save the current ROI 
                // width and height.  Otherwise, save the current width and
                // height of the image from the camera.
                if (setROI) {
                    tempMaxSizeX = roiArraySizeX;
                    tempMaxSizeY = roiArraySizeY;
                }
                else {
                    tempMaxSizeX = cameraArraySizeX;
                    tempMaxSizeY = cameraArraySizeY;
                }
                
                if(isDebugMessages)
                    IJ.log("ROI values: X=" + tempRoiX + ", Y=" + tempRoiY + ", W="
                                + tempRoiWidth + ", H=" + tempRoiHeight + ", MaxX="
                                + tempMaxSizeX + ", MaxY=" + tempMaxSizeY);

                calculateROI(tempRoiX, tempRoiY, tempRoiWidth, tempRoiHeight, tempMaxSizeX, tempMaxSizeY);
                
            } catch (Exception ex) {
                logMessage("Cannot set ROI:" + ex.getMessage(), true, true);
            }
        }
    }

    public void makeImageCopy() {
        ImageProcessor dipcopy = img.getProcessor().duplicate();
        ImagePlus imgcopy = new ImagePlus(imagePrefix + ":" + UniqueId, dipcopy);
        imgcopy.show();
    }

    /**
     * This method creates the PV objects for the camera.
     */
    public void connectCameraPVs() {
        try {
            cameraPrefix = cameraPrefixText.getText();
            logMessage("Trying to connect to EPICS PVs: " + cameraPrefix, true, true);
            if (isDebugFile) {
                debugPrintStream.println("Trying to connect to EPICS PVs: " + cameraPrefix);
                debugPrintStream.println("context.printfInfo  ****************************");
                debugPrintStream.println();
                ctxt.printInfo(debugPrintStream);

                debugPrintStream.print("jca.printInfo  ****************************");
                debugPrintStream.println();
                jca.printInfo(debugPrintStream);
                debugPrintStream.print("jca.listProperties  ****************************");
                debugPrintStream.println();
                jca.listProperties(debugPrintStream);
            }
            
            ch_minCamX = createEPICSChannel(cameraPrefix + "MinX");
            ch_minCamY = createEPICSChannel(cameraPrefix + "MinY");
            ch_minCamX_RBV = createEPICSChannel(cameraPrefix + "MinX_RBV");
            ch_minCamY_RBV = createEPICSChannel(cameraPrefix + "MinY_RBV");
            ch_sizeCamX = createEPICSChannel(cameraPrefix + "SizeX");
            ch_sizeCamY = createEPICSChannel(cameraPrefix + "SizeY");
            ch_sizeCamArrayX_RBV = createEPICSChannel(cameraPrefix + "ArraySizeX_RBV");
            ch_sizeCamArrayY_RBV = createEPICSChannel(cameraPrefix + "ArraySizeY_RBV");
            ch_maxSizeCamX = createEPICSChannel(cameraPrefix + "MaxSizeX_RBV");
            ch_maxSizeCamY = createEPICSChannel(cameraPrefix + "MaxSizeY_RBV");
            ch_camBinX_RBV = createEPICSChannel(cameraPrefix + "BinX_RBV");
            ch_camBinY_RBV = createEPICSChannel(cameraPrefix + "BinY_RBV");
            ch_sizeCamArrayX_RBV.addMonitor(Monitor.VALUE, new newCameraArraySizeX());
            ch_sizeCamArrayY_RBV.addMonitor(Monitor.VALUE, new newCameraArraySizeY());
            
            ctxt.flushIO();
            checkCameraPVConnections();
            
        } catch (Exception ex) {
            logMessage("CAException: Cannot connect to EPICS camera PV:" + ex.getMessage(), true, true);
            checkCameraPVConnections();
        }
    }
    
    /**
     * This method creates the PV objects for the transform plugin.
     */
    public void connectTransformPVs() {
        try {
            transformPrefix = transformPrefixText.getText();
            logMessage("Trying to connect to EPICS PVs: " + transformPrefix, true, true);
            if (isDebugFile) {
                debugPrintStream.println("Trying to connect to EPICS PVs: " + transformPrefix);
                debugPrintStream.println("context.printfInfo  ****************************");
                debugPrintStream.println();
                ctxt.printInfo(debugPrintStream);

                debugPrintStream.print("jca.printInfo  ****************************");
                debugPrintStream.println();
                jca.printInfo(debugPrintStream);
                debugPrintStream.print("jca.listProperties  ****************************");
                debugPrintStream.println();
                jca.listProperties(debugPrintStream);
            }
            
            ch_transType = createEPICSChannel(transformPrefix + "Type");

            ctxt.flushIO();
            checkTransformPVConnections();
            
        } catch (Exception ex) {
            logMessage("CAException: Cannot connect to EPICS transform PV:" + ex.getMessage(), true, true);
            checkTransformPVConnections();
        }
    }
    
    /**
     * This method creates the PV objects for the ROI plugin.
     */
    public void connectRoiPVs() {
        try {
            roiPrefix = roiPrefixText.getText();
            logMessage("Trying to connect to EPICS PVs: " + roiPrefix, true, true);
            if (isDebugFile) {
                debugPrintStream.println("Trying to connect to EPICS PVs: " + roiPrefix);
                debugPrintStream.println("context.printfInfo  ****************************");
                debugPrintStream.println();
                ctxt.printInfo(debugPrintStream);

                debugPrintStream.print("jca.printInfo  ****************************");
                debugPrintStream.println();
                jca.printInfo(debugPrintStream);
                debugPrintStream.print("jca.listProperties  ****************************");
                debugPrintStream.println();
                jca.listProperties(debugPrintStream);
            }
            
            ch_minRoiX = createEPICSChannel(roiPrefix + "MinX");
            ch_minRoiY = createEPICSChannel(roiPrefix + "MinY");
            ch_minRoiX_RBV = createEPICSChannel(roiPrefix + "MinX_RBV");
            ch_minRoiY_RBV = createEPICSChannel(roiPrefix + "MinY_RBV");
            ch_sizeRoiX = createEPICSChannel(roiPrefix + "SizeX");
            ch_sizeRoiY = createEPICSChannel(roiPrefix + "SizeY");
            ch_roiBinX_RBV = createEPICSChannel(roiPrefix + "BinX_RBV");
            ch_roiBinY_RBV = createEPICSChannel(roiPrefix + "BinY_RBV");
            ch_sizeRoiArrayX_RBV = createEPICSChannel(roiPrefix + "ArraySizeX_RBV");
            ch_sizeRoiArrayY_RBV = createEPICSChannel(roiPrefix + "ArraySizeY_RBV");
            ch_sizeRoiArrayX_RBV.addMonitor(Monitor.VALUE, new newRoiArraySizeX());
            ch_sizeRoiArrayY_RBV.addMonitor(Monitor.VALUE, new newRoiArraySizeY());
            
            ctxt.flushIO();
            checkRoiPVConnections();
            
        } catch (Exception ex) {
            logMessage("CAException: Cannot connect to EPICS ROI PV:" + ex.getMessage(), true, true);
            checkRoiPVConnections();
        }  
    }

    /**
     * This method destroys the PV objects for the camera.
     */
    public void disconnectCameraPVs() {
        try {
            ch_minCamX.destroy();
            ch_minCamY.destroy();
            ch_minCamX_RBV.destroy();
            ch_minCamY_RBV.destroy();
            ch_sizeCamX.destroy();
            ch_sizeCamY.destroy();
            ch_sizeCamArrayX_RBV.destroy();
            ch_sizeCamArrayY_RBV.destroy();
            ch_maxSizeCamX.destroy();
            ch_maxSizeCamY.destroy();
            ch_camBinX_RBV.destroy();
            ch_camBinY_RBV.destroy();
            isCameraConnected = false;
            
            logMessage("Disconnected from EPICS camera PVs OK", true, true);
        }catch (CAException ex) {
            logMessage("CAException: Cannot disconnect from EPICS camera PV:" + ex.getMessage(), true, true);
        } catch (IllegalStateException ex) {
            logMessage("IllegalStateException: Cannot disconnect from EPICS camera PV:" + ex.getMessage(), true, true);
        } catch (NullPointerException ex) {
            logMessage("NullPointerException: Cannot disconnect from EPICS camera PV:" + ex.getMessage(), true, true);
        }
    }
    
    /**
     * This method destroys the PV objects for the transform plugin.
     */
    public void disconnectTransformPVs() {
        try {
            ch_transType.destroy();
            isTransformConnected = false;
            
            logMessage("Disconnected from EPICS transform PVs OK", true, true);
        } catch (CAException ex) {
            logMessage("CAException: Cannot disconnect from EPICS transform PV:" + ex.getMessage(), true, true);
        } catch (IllegalStateException ex) {
            logMessage("IllegalStateException: Cannot disconnect from EPICS tansform PV:" + ex.getMessage(), true, true);
        } catch (NullPointerException ex) {
            logMessage("NullPointerException: Cannot disconnect from EPICS transform PV:" + ex.getMessage(), true, true);
        }
    }
    
    /**
     * This method destroys the PV objects for the ROI plugin.
     */
    public void disconnectRoiPVs() {
        try {
            ch_minRoiX.destroy();
            ch_minRoiY.destroy();
            ch_minRoiX_RBV.destroy();
            ch_minRoiY_RBV.destroy();
            ch_sizeRoiX.destroy();
            ch_sizeRoiY.destroy();
            ch_sizeRoiArrayX_RBV.destroy();
            ch_sizeRoiArrayY_RBV.destroy();
            ch_roiBinX_RBV.destroy();
            ch_roiBinY_RBV.destroy();
            isRoiConnected = false;
            
            logMessage("Disconnected from EPICS ROI PVs OK", true, true);
        } catch (CAException ex) {
            logMessage("CAException: Cannot disconnect from EPICS ROI PV:" + ex.getMessage(), true, true);
        } catch (IllegalStateException ex) {
            logMessage("IllegalStateException: Cannot disconnect from EPICS ROI PV:" + ex.getMessage(), true, true);
        } catch (NullPointerException ex) {
            logMessage("NullPointerException: Cannot disconnect from EPICS ROI PV:" + ex.getMessage(), true, true);
        }
    }

    /**
     * This method creates the channel access context and initializes it.
     */
    public void startEPICSCA() {
        logMessage("Initializing EPICS", true, true);

        try {
            System.setProperty("jca.use_env", "true");
            // Get the JCALibrary instance.
            jca = JCALibrary.getInstance();
            ctxt = jca.createContext(JCALibrary.CHANNEL_ACCESS_JAVA);
            ctxt.initialize();
        } catch (CAException ex) {
            logMessage("startEPICSCA exception: " + ex.getMessage(), true, true);
        }

    }

    /**
     * This method destroys the channel access context.
     * 
     * @throws Exception 
     */
    public void closeEPICSCA() throws Exception {
        logMessage("Closing EPICS", true, true);
        ctxt.destroy();
    }

    /**
     * This method takes a string argument, creates a Channel object, and
     * returns it.
     * 
     * @param chname      A string that is the name of the PV.
     * @return
     * @throws Exception 
     */
    public Channel createEPICSChannel(String chname) throws Exception {
        // Create the Channel to connect to the PV.
        Channel ch = ctxt.createChannel(chname);

        // send the request and wait for the channel to connect to the PV.
        ctxt.pendIO(2.0);
        if (isDebugFile) {
            debugPrintStream.print("\n\n  Channel info****************************\n");
            ch.printInfo(debugPrintStream);
        }
        if (isDebugMessages) {
            IJ.log("Host is " + ch.getHostName());
            IJ.log("can read = " + ch.getReadAccess());
            IJ.log("can write " + ch.getWriteAccess());
            IJ.log("type " + ch.getFieldType());
            IJ.log("name = " + ch.getName());
            IJ.log("element count = " + ch.getElementCount());
        }
        return (ch);
    }

    /**
     * This method is called whenever the size of the data array in the X
     * dimension changes.  It reads the X value of the start of the camera's
     * readout and the size of the data array and saves those values.
     */
    public class newCameraArraySizeX implements MonitorListener {
        
        @Override
        public void monitorChanged(MonitorEvent ev) {
            if (isDebugMessages)
                IJ.log("New size for " + cameraPrefix + "ArraySizeX_RBV");
            
            DBR_Int sizeX = (DBR_Int) ev.getDBR();
            cameraArraySizeX = (sizeX.getIntValue())[0];
            
            try {
                ch_minCamX_RBV.get(DBRType.INT, 1, new GetListener() {
                
                    @Override
                    public void getCompleted (GetEvent ev) {
                        DBR_Int x = (DBR_Int) ev.getDBR();
                        cameraOriginX = (x.getIntValue())[0];
                        if (isDebugMessages)
                           IJ.log("Camera X origin at " + cameraOriginX);
                    }
                });
            } catch (CAException ex) {
                logMessage("newCameraArraySizeX CAException: Cannot read " + cameraPrefix + "MinX_RBV:" + ex.getMessage(), true, true);
            } catch (IllegalStateException ex) {
                logMessage("newCameraArraySizeX IllegalStateException: Cannot read " + cameraPrefix + "MinX_RBV:" + ex.getMessage(), true, true);
            }
                        
            try {
                ch_camBinX_RBV.get(DBRType.INT, 1, new GetListener() {
                
                    @Override
                    public void getCompleted (GetEvent ev) {
                        DBR_Int x = (DBR_Int) ev.getDBR();
                        cameraBinX = (x.getIntValue())[0];
                        if (isDebugMessages)
                           IJ.log("Camera X bin value =  " + cameraBinX);
                    }
                });
            } catch (CAException ex) {
                logMessage("newCameraArraySizeX CAException: Cannot read " + cameraPrefix + "BinX_RBV:" + ex.getMessage(), true, true);
            } catch (IllegalStateException ex) {
                logMessage("newCameraArraySizeX IllegalStateException: Cannot read " + cameraPrefix + "BinX_RBV:" + ex.getMessage(), true, true);
            }
        }
    }
    
    /**
     * This method is called whenever the size of the data array in the Y
     * dimension changes.  It reads the Y value of the start of the camera's
     * readout and the size of the data array and saves those values.
     */
    public class newCameraArraySizeY implements MonitorListener {
        
        @Override
        public void monitorChanged(MonitorEvent ev) {
            if (isDebugMessages)
                IJ.log("New size for " + cameraPrefix + "ArraySizeY_RBV");
            
            DBR_Int sizeY = (DBR_Int) ev.getDBR();
            cameraArraySizeY = (sizeY.getIntValue())[0];
            
            try {
                ch_minCamY_RBV.get(DBRType.INT, 1, new GetListener() {
                
                    @Override
                    public void getCompleted (GetEvent ev) {
                        DBR_Int y = (DBR_Int) ev.getDBR();
                        cameraOriginY = (y.getIntValue())[0];
                        if (isDebugMessages)
                            IJ.log("Camera Y origin at " + cameraOriginY);
                    }
                });
            } catch (CAException ex) {
                logMessage("newCameraArraySizeY CAException: Cannot read " + cameraPrefix + "MinY_RBV:" + ex.getMessage(), true, true);
            } catch (IllegalStateException ex) {
                logMessage("newCameraArraySizeY IllegalStateException: Cannot read " + cameraPrefix + "MinY_RBV:" + ex.getMessage(), true, true);
            }
                        
            try {
                ch_camBinY_RBV.get(DBRType.INT, 1, new GetListener() {
                
                    @Override
                    public void getCompleted (GetEvent ev) {
                        DBR_Int y = (DBR_Int) ev.getDBR();
                        cameraBinY = (y.getIntValue())[0];
                        if (isDebugMessages)
                            IJ.log("Camera Y bin value = " + cameraBinY);
                    }
                });
            } catch (CAException ex) {
                logMessage("newCameraArraySizeY CAException: Cannot read " + cameraPrefix + "BinY_RBV:" + ex.getMessage(), true, true);
            } catch (IllegalStateException ex) {
                logMessage("newCameraArraySizeY IllegalStateException: Cannot read " + cameraPrefix + "BinY_RBV:" + ex.getMessage(), true, true);
            }
        }
    }
    
    /**
     * This method is called whenever the size of the data array for the ROI
     * plugin in the X dimension changes.  It reads the X value of the start
     * of the ROI and the size of the data array and saves those values.
     */
    public class newRoiArraySizeX implements MonitorListener {
        
        @Override
        public void monitorChanged(MonitorEvent ev) {
            if (isDebugMessages)
                IJ.log("New size for " + roiPrefix + "ArraySizeX_RBV");
            
            DBR_Int sizeX = (DBR_Int) ev.getDBR();
            roiArraySizeX = (sizeX.getIntValue())[0];
            
            try {
                ch_minRoiX_RBV.get(DBRType.INT, 1, new GetListener() {
                
                    @Override
                    public void getCompleted (GetEvent ev) {
                        DBR_Int x = (DBR_Int) ev.getDBR();
                        roiOriginX = (x.getIntValue())[0];
                        if (isDebugMessages)
                           IJ.log("ROI X origin at " + roiOriginX);
                    }
                });
            } catch (CAException ex) {
                logMessage("newRoiArraySizeX CAException: Cannot read " + roiPrefix + "MinX_RBV:" + ex.getMessage(), true, true);
            } catch (IllegalStateException ex) {
                logMessage("newRoiArraySizeX IllegalStateException: Cannot read " + roiPrefix + "MinX_RBV:" + ex.getMessage(), true, true);
            }
                        
            try {
                ch_roiBinX_RBV.get(DBRType.INT, 1, new GetListener() {
                
                    @Override
                    public void getCompleted (GetEvent ev) {
                        DBR_Int x = (DBR_Int) ev.getDBR();
                        roiBinX = (x.getIntValue())[0];
                        if (isDebugMessages)
                           IJ.log("ROI X bin value = " + roiBinX);
                    }
                });
            } catch (CAException ex) {
                logMessage("newRoiArraySizeX CAException: Cannot read " + roiPrefix + "BinX_RBV:" + ex.getMessage(), true, true);
            } catch (IllegalStateException ex) {
                logMessage("newRoiArraySizeX IllegalStateException: Cannot read " + roiPrefix + "BinX_RBV:" + ex.getMessage(), true, true);
            }
        }
    }
    
    /**
     * This method is called whenever the size of the data array for the ROI
     * plugin in the Y dimension changes.  It reads the Y value of the start
     * of the ROI and the size of the data array and saves those values.
     */
    public class newRoiArraySizeY implements MonitorListener {
        
        @Override
        public void monitorChanged(MonitorEvent ev) {
            if (isDebugMessages)
                IJ.log("New size for " + roiPrefix + "ArraySizeY_RBV");
            
            DBR_Int sizeY = (DBR_Int) ev.getDBR();
            roiArraySizeY = (sizeY.getIntValue())[0];
            
            try {
                ch_minRoiY_RBV.get(DBRType.INT, 1, new GetListener() {
                
                    @Override
                    public void getCompleted (GetEvent ev) {
                        DBR_Int y = (DBR_Int) ev.getDBR();
                        roiOriginY = (y.getIntValue())[0];
                        if (isDebugMessages)
                           IJ.log("ROI Y origin at " + roiOriginY);
                    }
                });
            } catch (CAException ex) {
                logMessage("newRoiArraySizeY CAException: Cannot read " + roiPrefix + "MinY_RBV:" + ex.getMessage(), true, true);
            } catch (IllegalStateException ex) {
                logMessage("newRoiArraySizeY IllegalStateException: Cannot read " + roiPrefix + "MinY_RBV:" + ex.getMessage(), true, true);
            }
                        
            try {
                ch_roiBinY_RBV.get(DBRType.INT, 1, new GetListener() {
                
                    @Override
                    public void getCompleted (GetEvent ev) {
                        DBR_Int x = (DBR_Int) ev.getDBR();
                        roiBinY = (x.getIntValue())[0];
                        if (isDebugMessages)
                           IJ.log("ROI Y bin value = " + roiBinX);
                    }
                });
            } catch (CAException ex) {
                logMessage("newRoiArraySizeY CAException: Cannot read " + roiPrefix + "BinY_RBV:" + ex.getMessage(), true, true);
            } catch (IllegalStateException ex) {
                logMessage("newRoiArraySizeY IllegalStateException: Cannot read " + roiPrefix + "BinY_RBV:" + ex.getMessage(), true, true);
            }
        }
    }

    /**
     * This method checks that the PV objects for the camera both exist
     * and are connected to the underlying PVs.
     */
    public void checkCameraPVConnections() {
        boolean cameraConnected;
        try {
            cameraConnected = (ch_minCamX != null && ch_minCamX.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_minCamY != null && ch_minCamY.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_minCamX_RBV != null && ch_minCamX_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_minCamY_RBV != null && ch_minCamY_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_sizeCamX != null && ch_sizeCamX.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_sizeCamY != null && ch_sizeCamY.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_maxSizeCamX != null && ch_maxSizeCamX.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_maxSizeCamY != null && ch_maxSizeCamY.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_camBinX_RBV != null && ch_camBinX_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_camBinY_RBV != null && ch_camBinY_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_sizeCamArrayX_RBV != null && ch_sizeCamArrayX_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_sizeCamArrayY_RBV != null && ch_sizeCamArrayY_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED);

            if (cameraConnected & !isCameraConnected) {
                isCameraConnected = true;
                logMessage("Connection to EPICS camera PVs OK", true, true);
                cameraPrefixText.setBackground(Color.green);
                setCCDReadoutRadioButton.setEnabled(true);
                resetRoiButton.setEnabled(true);
                
                cameraOriginX = epicsGetInt(ch_minCamX_RBV);
                cameraOriginY = epicsGetInt(ch_minCamY_RBV);
                cameraArraySizeX = epicsGetInt(ch_sizeCamArrayX_RBV);
                cameraArraySizeY = epicsGetInt(ch_sizeCamArrayY_RBV);
                cameraBinX = epicsGetInt(ch_camBinX_RBV);
                cameraBinY = epicsGetInt(ch_camBinY_RBV);
                
            }

            if (!cameraConnected) {
                isCameraConnected = false;
                logMessage("Cannot connect to EPICS camera PVs", true, true);
                cameraPrefixText.setBackground(Color.red);
                setCCDReadoutRadioButton.setSelected(false);
                setCCDReadoutRadioButton.setEnabled(false);
                resetRoiButton.setEnabled(false);
                setCCD_Readout = false;
            }


        } catch (IllegalStateException ex) {
            IJ.log("checkCameraPVConnections: got IllegalStateException= " + ex.getMessage());
            if (isDebugFile) {
                ex.printStackTrace(debugPrintStream);
            }
        } catch (TimeoutException ex) {
            IJ.log("checkCameraPVConnections: got TimeoutException= " + ex.getMessage());
            if (isDebugFile) {
                ex.printStackTrace(debugPrintStream);
            }
        } catch (CAException ex) {
            IJ.log("checkCameraPVConnections: got CAException= " + ex.getMessage());
            if (isDebugFile) {
                ex.printStackTrace(debugPrintStream);
            }
        }
    }
    
    /**
     * This method checks that the PV objects for the transform plugin both exist
     * and are connected to the underlying PVs.
     */
    public void checkTransformPVConnections() {
        boolean transformConnected;
        
        try {
/*            transformConnected = (ch_transType[0] != null && ch_transType[0].getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_transType[1] != null && ch_transType[1].getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_transType[2] != null && ch_transType[2].getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_transType[3] != null && ch_transType[3].getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_origin != null && ch_origin.getConnectionState() == Channel.ConnectionState.CONNECTED);
*/
            transformConnected = (ch_transType != null && ch_transType.getConnectionState() == Channel.ConnectionState.CONNECTED);
            if (transformConnected & !isTransformConnected) {
                isTransformConnected = true;
                logMessage("Connection to EPICS transform PVs OK", true, true);
                transformPrefixText.setBackground(Color.green);
                useTransformCheckBox.setEnabled(true);
            }

            if (!transformConnected) {
                isTransformConnected = false;
                logMessage("Cannot connect to EPICS transform PVs", true, true);
                transformPrefixText.setBackground(Color.red);
                useTransformCheckBox.setSelected(false);
                useTransformCheckBox.setEnabled(false);
                useTransformPlugin = false;
            }

        } catch (IllegalStateException ex) {
            IJ.log("checkTransformPVConnections: got exception= " + ex.getMessage());
            if (isDebugFile) {
                ex.printStackTrace(debugPrintStream);
            }
        }
    }
    
    /**
     * This method checks that the PV objects for the ROI plugin both exist
     * and are connected to the underlying PVs.
     */
    public void checkRoiPVConnections() {
        boolean roiConnected;

        try {
            roiConnected = (ch_minRoiX != null && ch_minRoiX.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_minRoiY != null && ch_minRoiY.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_minRoiX_RBV != null && ch_minRoiX_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_minRoiY_RBV != null && ch_minRoiY_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_sizeRoiX != null && ch_sizeRoiX.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_sizeRoiY != null && ch_sizeRoiY.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_roiBinX_RBV != null && ch_roiBinX_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_roiBinY_RBV != null && ch_roiBinY_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_sizeRoiArrayX_RBV != null && ch_sizeRoiArrayX_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_sizeRoiArrayY_RBV != null && ch_sizeRoiArrayY_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED);

            if (roiConnected & !isRoiConnected) {
                isRoiConnected = true;
                logMessage("Connection to EPICS ROI PVs OK", true, true);
                roiPrefixText.setBackground(Color.green);
                setROIRadioButton.setEnabled(true);
                preTransformRadioButton.setEnabled(true);
                postTransformRadioButton.setEnabled(true);
                
                roiOriginX = epicsGetInt(ch_minRoiX_RBV);
                roiOriginY = epicsGetInt(ch_minRoiY_RBV);
                roiArraySizeX = epicsGetInt(ch_sizeRoiArrayX_RBV);
                roiArraySizeY = epicsGetInt(ch_sizeRoiArrayY_RBV);
                roiBinX = epicsGetInt(ch_roiBinX_RBV);
                roiBinY = epicsGetInt(ch_roiBinY_RBV);
            }

            if (!roiConnected) {
                isRoiConnected = false;
                logMessage("Cannot connect to EPICS ROI PVs", true, true);
                roiPrefixText.setBackground(Color.red);
                setROIRadioButton.setSelected(false);
                setROIRadioButton.setEnabled(false);
                preTransformRadioButton.setEnabled(false);
                postTransformRadioButton.setEnabled(false);
            }
            
        } catch (IllegalStateException ex) {
            IJ.log("checkROIPVConnections: got exception= " + ex.getMessage());
            if (isDebugFile) {
                ex.printStackTrace(debugPrintStream);
            }
        } catch (TimeoutException ex) {
            IJ.log("checkROIPVConnections: got exception= " + ex.getMessage());
            if (isDebugFile) {
                ex.printStackTrace(debugPrintStream);
            }
        } catch (CAException ex) {
            IJ.log("checkROIPVConnections: got exception= " + ex.getMessage());
            if (isDebugFile) {
                ex.printStackTrace(debugPrintStream);
            }
        }
    }

    /**
     * Read an integer value from a PV and return it.
     * 
     * @param ch    A channel object that is used to read the PV.
     * @return
     * @throws gov.aps.jca.TimeoutException
     * @throws gov.aps.jca.CAException
     * @throws IllegalStateException
     */
    public int epicsGetInt(Channel ch) throws TimeoutException, CAException, IllegalStateException {
        if (isDebugMessages) {
            IJ.log("Channel Get: " + ch.getName());
        }
        DBR_Int x = (DBR_Int) ch.get(DBRType.INT, 1);
        ctxt.pendIO(5.0);
        return (x.getIntValue()[0]);
    }

    /**
     * Read an enum (short int) value from a PV and return it.
     * 
     * @param ch   A channel object that is used to read the PV.
     * @return
     * @throws gov.aps.jca.TimeoutException
     * @throws gov.aps.jca.CAException
     * @throws IllegalStateException 
     */
    public short epicsGetEnum(Channel ch) throws TimeoutException, CAException, IllegalStateException {
        if (isDebugMessages) {
            IJ.log("Channel Get: " + ch.getName());
        }
        DBR_Enum x = (DBR_Enum) ch.get(DBRType.ENUM, 1);
        ctxt.pendIO(5.0);
        return (x.getEnumValue()[0]);
    }

    /**
     * Write an integer value to a PV.
     * 
     * @param ch    A channel object used to write to the PV.
     * @param num   The integer value to be written.
     * @throws gov.aps.jca.TimeoutException
     * @throws gov.aps.jca.CAException
     * @throws IllegalStateException 
     */
    public void epicsSetInt(Channel ch, int num) throws TimeoutException, CAException, IllegalStateException {
        ch.put(num, new PutListener() {
            @Override
            public void putCompleted(PutEvent ev) {
                if (isDebugMessages)
                    IJ.log("Put operation complete");
            }
        });
        ctxt.pendIO(5.0);
    }

    /**
     * This method writes the values for the user drawn rectangle to either the
     * camera or the ROI plugin.
     * 
     * @param chX      This is the PV for the X value of the rectangle's origin.
     * @param chY      This is the PV for the Y value of the rectangle's origin.
     * @param chSizeX  This is the PV for the size of the rectangle in X.
     * @param chSizeY  This is the PV for the size of the rectangle in Y.
     * @param x        This is the value to write to chX.
     * @param y        This is the value to write to chY.
     * @param sizeX    This is the value to write to chSizeX.
     * @param sizeY    This is the value to write to chSizeY.
     */
    public void setROI(Channel chX, Channel chY, Channel chSizeX, Channel chSizeY,
            int x, int y, int sizeX, int sizeY) {
        try {
            epicsSetInt(chX, x);           
            epicsSetInt(chY, y);
            epicsSetInt(chSizeX, sizeX);
            epicsSetInt(chSizeY, sizeY);
        } catch (CAException ex) {
            logMessage("setROI got CAException: " + ex.getMessage(), true, true);
        } catch (TimeoutException ex) {
            logMessage("setROI got TimeoutException: " + ex.getMessage(), true, true);
        } catch (IllegalStateException ex) {
            logMessage("setROI got IllegalStateException: " + ex.getMessage(), true, true);
        }
    }

    /**
     * This class is used to determine the location of the origin point
     * and the orientation of the X and Y axes.
     */
    private class OriginParameters {

        private short origin;
        //private short[] transformType = new short[4];
        private short transformType;
        private boolean flipAxes;
        private int ii;

        public OriginParameters() {
            try {
                origin = UPPER_LEFT;
                flipAxes = false;

                transformType = epicsGetEnum(ch_transType);
            } catch (CAException ex) {
                IJ.log("OriginParameters CAException: Could not get parameters from " + transformPrefix + ": " + ex.getMessage());
            } catch (TimeoutException ex) {
                IJ.log("OriginParameters TimeoutException: Could not get parameters from " + transformPrefix + ": " + ex.getMessage());
            } catch (IllegalStateException ex) {
                IJ.log("OriginParameters IllegalStateException: Could not get parameters from " + transformPrefix + ": " + ex.getMessage());
            }
        }

        /**
         * This method determines where the origin of the image is with respect to
         * what is displayed by this plugin.  It also determines whether or not the
         * X and Y axes have been flipped so that the X axis is now vertical and the
         * Y axis is now horizontal.
         * <p>
         * It iterates through all of the transforms types that the area detector
         * Transform plugin performs and moves the origin location and axis
         * orientation accordingly.  The results can then be obtained through two
         * get methods.
         * 
         * @see getOrigin()
         * @see isFlippedAxes()
         */
        public void findOriginLocation() {
                switch (transformType) {
                    
                    // This image is not altered by the transform.
                    case NONE:
                        break;

                    // Rotate the image 90 degrees clockwise.
                    case ROTATE_90:
                        flipAxes = !flipAxes;
                        break;

                    // Rotate the image 90 degrees counter clockwise.
                    case ROTATE_270:
                        flipAxes = !flipAxes;
                        break;

                    // Rotate the image 180 degrees.
                    case ROTATE_180:
                        break;

                    // Flip the image along the diagonal that goes from 0,0 to
                    // maxX, maxY.  This does not cause the origin location to
                    // change.
                    case ROTATE_90_MIRROR:
                        flipAxes = !flipAxes;
                        break;

                    // Flip the image along the diagonal that goes from 0, maxY
                    // to maxX, 0.
                   case ROTATE_270_MIRROR:
                        flipAxes = !flipAxes;
                        break;

                    // Flip x values of the image pixels.
                   case MIRROR:
                        break;

                    // Flip the y values of the image pixels.
                   case ROTATE_180_MIRROR:
                        break;

                    default:
                        logMessage("findOriginLocation: found no matching transform type", true, true);
                        break;
                }
            if (isDebugMessages)
                logMessage("findOriginLocation: origin location is " + origin, true, true);
        }

        public short getOrigin() {
            return origin;
        }

        public boolean isFlippedAxes() {
            return flipAxes;
        }
    }

    /**
     * This method calculates the values that will be written to the area detector
     * software.  It calculates a starting point for the X and Y coordinates based
     * on the location of the origin point with respect to the image.  If the user
     * has checked the use Transform Plugin checkbox, then it calculates the origin
     * based on the AD array transforms performed.  It will also determine if the X
     * and Y axes have been flipped.  Otherwise, it assumes that the origin is at
     * the upper right corner of the image and the axes have not been flipped.  It
     * will write its values to either the cameras readout regions PVs, or to a ROI
     * plugin, depending on the user's choice.
     * 
     * @param startX    the starting point in X of the rectangle drawn on the image
     * @param startY    the starting point in X of the rectangle drawn on the image
     * @param sizeX     the size of the rectangle in the X dimension
     * @param sizeY     the size of the rectangle in the Y dimension
     * @param maxSizeX  the maximum size of the current image in the X dimension
     * @param maxSizeY  the maximum size of the current image in the Y dimension
     */
    public void calculateROI(int startX, int startY, int sizeX, int sizeY, int maxSizeX, int maxSizeY) {
        short origin = UPPER_LEFT;
        boolean flipAxes = false;
        OriginParameters params;
        
        int tempX = 0;
        int tempY = 0;
        int tempSizeX = 0;
        int tempSizeY = 0;
        
        if (useTransformPlugin  && (!isPostTransform))
        {
            params = this.new OriginParameters();
            params.findOriginLocation();
            origin = params.getOrigin();
            flipAxes = params.isFlippedAxes();
            
            if (isDebugMessages) {
                IJ.log("The origin=" + origin);
                IJ.log("Axes are flipped=" + flipAxes);
            }
        }
        
        
        
        /* If the user wanted to set the ROI plugin and
         * the ROI gets its data from the transform
         * plugin, then the axes should not be flipped.
         */
        /*if (setROI && isPostTransform)
            flipAxes = false;*/
        
        switch (origin)
        {
            case UPPER_LEFT:
                if (flipAxes)
                {
                    tempX = startY;
                    tempY = startX;
                    tempSizeX = sizeY;
                    tempSizeY = sizeX;
                }
                else
                {
                    tempX = startX;
                    tempY = startY;
                    tempSizeX = sizeX;
                    tempSizeY = sizeY;
                }
                break;
            default:
                break;
        }
        
        if (isDebugMessages) {
            IJ.log("The new calculated value of tempX is " + tempX);
            IJ.log("The new calculated value of tempY is " + tempY);
        }
                
        // Check to make sure that the sizes are not less than zero.
        if (tempSizeX < 0)
            tempSizeX = 0;
        
        if (tempSizeY < 0)
            tempSizeY = 0;
        
        try {
            if (setCCD_Readout) {
                if (isDebugMessages) {
                    IJ.log("The value of cameraOriginX is " + cameraOriginX);
                    IJ.log("The value of cameraOriginY is " + cameraOriginY);
                }
                
                // The user may have entered a value for binning.
                // Calulate the new value of tempX  and tempSizeX with binning.
                tempX = ((tempX - 1) * cameraBinX) + 1;
                tempSizeX = tempSizeX * cameraBinX;
                tempY = ((tempY - 1) * cameraBinY) + 1;
                tempSizeY = tempSizeY * cameraBinY;
                
                // Check to make sure that the values are not less than zero.
                if (tempX < 0)
                    tempX = 0;
        
                if (tempY < 0)
                    tempY = 0;

                tempX = tempX + cameraOriginX;
                tempY = tempY + cameraOriginY;
                
                setROI(ch_minCamX, ch_minCamY, ch_sizeCamX, ch_sizeCamY, tempX,
                            tempY, tempSizeX, tempSizeY);
            }
            
            if (setROI) {
                if (isDebugMessages) {
                    IJ.log("The value of roiOriginX is " + roiOriginX);
                    IJ.log("The value of roiOriginY is " + roiOriginY);
                }
                
                // The user may have entered a value for binning.
                // Calulate the new value of tempX  and tempSizeX with binning.
                tempX = ((tempX - 1) * roiBinX) + 1;
                tempSizeX = tempSizeX * roiBinX;
                tempY = ((tempY - 1) * roiBinY) + 1;
                tempSizeY = tempSizeY * roiBinY;
                
                // Check to make sure that the values are not less than zero.
                if (tempX < 0)
                    tempX = 0;
        
                if (tempY < 0)
                    tempY = 0;

                tempX = tempX + roiOriginX;
                tempY = tempY + roiOriginY;
                
                setROI(ch_minRoiX, ch_minRoiY, ch_sizeRoiX, ch_sizeRoiY, tempX,
                            tempY, tempSizeX, tempSizeY);
            }
        } catch (Exception ex) {
            logMessage("calculateROI: cannot get origin X and Y " + origin, true, true);
        }
        if (isDebugMessages) {
            IJ.log("The final calculated value of tempX is " + tempX);
            IJ.log("The final calculated value of tempY is " + tempY);
        }
    }

    /**
     * Create the GUI and show it. For thread safety, this method should be
     * invoked from the event-dispatching thread.
     */
    public void createAndShowGUI() {
        //Create and set up the window.
        NXText = new JTextField(6);
        NXText.setEditable(false);
        NXText.setHorizontalAlignment(JTextField.CENTER);
        NYText = new JTextField(6);
        NYText.setEditable(false);
        NYText.setHorizontalAlignment(JTextField.CENTER);
        NZText = new JTextField(6);
        NZText.setEditable(false);
        NZText.setHorizontalAlignment(JTextField.CENTER);
        FPSText = new JTextField(6);
        FPSText.setEditable(false);
        FPSText.setHorizontalAlignment(JTextField.CENTER);
        StatusText = new JTextField(40);
        StatusText.setEditable(false);
        
        JButton increaseWidthButton = new JButton("W +");
        JButton decreaseWidthButton = new JButton("W -");
        JButton increaseHeightButton = new JButton("H +");
        JButton decreaseHeightButton = new JButton("H -");
        JButton increaseXButton = new JButton("X +");
        JButton decreaseXButton = new JButton("X -");
        JButton increaseYButton = new JButton("Y +");
        JButton decreaseYButton = new JButton("Y -");
        

        cameraPrefixText = new JTextField(cameraPrefix, 15);
        transformPrefixText = new JTextField(transformPrefix, 15);
        roiPrefixText = new JTextField(roiPrefix, 15);
        tweakAmountText = new JTextField(String.valueOf(tweakAmountPixels), 5);
        resetRoiButton = new JButton("Reset ROI");
        setNoneRadioButton = new JRadioButton("Set None",true);
        setCCDReadoutRadioButton = new JRadioButton("Set CCD Readout", false);
        setROIRadioButton = new JRadioButton("Set ROI Plugin", false);
        preTransformRadioButton = new JRadioButton("ROI Pre-Transform", false);
        preTransformRadioButton.setEnabled(false);
        postTransformRadioButton = new JRadioButton("ROI Post-Transform", false);
        postTransformRadioButton.setEnabled(false);
        useTransformCheckBox = new JCheckBox("Use Transform Plugin?", false);
        ButtonGroup groupA = new ButtonGroup();
        ButtonGroup groupB = new ButtonGroup();
        
        groupA.add(setNoneRadioButton);
        groupA.add(setCCDReadoutRadioButton);
        groupA.add(setROIRadioButton);
        
        groupB.add(preTransformRadioButton);
        groupB.add(postTransformRadioButton);
        
        JPanel groupAPanel = new JPanel();
        groupAPanel.setLayout(new BoxLayout(groupAPanel, BoxLayout.Y_AXIS));
        groupAPanel.add(setNoneRadioButton);
        groupAPanel.add(setCCDReadoutRadioButton);
        groupAPanel.add(setROIRadioButton);
        
        JPanel groupBPanel = new JPanel();
        groupBPanel.setLayout(new BoxLayout(groupBPanel, BoxLayout.Y_AXIS));
        groupBPanel.add(preTransformRadioButton);
        groupBPanel.add(postTransformRadioButton);
        

        frame = new JFrame("Image J EPICS_AD_Viewer Plugin");
        JPanel panel = new JPanel(new BorderLayout());
        panel.setLayout(new GridBagLayout());
        panel.setBorder(new EmptyBorder(new Insets(5, 5, 5, 5)));
        frame.getContentPane().add(BorderLayout.CENTER, panel);
        GridBagConstraints c = new GridBagConstraints();
        // Add extra space around each component to avoid clutter
        c.insets = new Insets(2, 2, 2, 2);

        // Top row
        // These widgets should be centered
        c.anchor = GridBagConstraints.CENTER;
        c.gridx = 0;
        c.gridy = 0;
        panel.add(new JLabel("Camera PV Prefix"), c);
        c.gridx = 1;
        panel.add(new JLabel("ROI PV Prefix"), c);
        c.gridx = 2;
        panel.add(new JLabel("Transform PV Prefix"), c);
        c.gridx = 4;
        panel.add(new JLabel("Tweak Amount"), c);
        c.gridx = 5;
        panel.add(tweakAmountText, c);

        // Second row
        c.anchor = GridBagConstraints.CENTER;
        c.gridy = 1;
        c.gridx = 0;
        panel.add(cameraPrefixText, c);
        c.gridx = 1;
        panel.add(roiPrefixText, c);
        c.gridx = 2;
        panel.add(transformPrefixText, c);
        c.gridx = 3;
        panel.add(resetRoiButton, c);
        c.gridx = 5;
        panel.add(increaseWidthButton, c);
        c.gridx = 6;
        panel.add(decreaseWidthButton, c);
        c.gridx = 7;
        panel.add(increaseHeightButton, c);
        c.gridx = 8;
        panel.add(decreaseHeightButton, c);

        // Third row
        c.anchor = GridBagConstraints.CENTER;
        c.gridy = 2;
        c.gridx = 0;
        panel.add(groupAPanel, c);
        c.gridx = 1;
        panel.add(groupBPanel, c);
        c.gridx = 2;
        panel.add(useTransformCheckBox, c);
        c.gridx = 5;
        panel.add(increaseXButton, c);
        c.gridx = 6;
        panel.add(decreaseXButton, c);
        c.gridx = 7;
        panel.add(increaseYButton, c);
        c.gridx = 8;
        panel.add(decreaseYButton, c);
        //panel.add(new JLabel("X Pos"), c);

        // Bottom row
        c.gridy = 5;
        c.gridx = 0;
        c.anchor = GridBagConstraints.EAST;
        panel.add(new JLabel("Status: "), c);
        c.gridx = 1;
        c.gridwidth = 7;
        c.anchor = GridBagConstraints.WEST;
        panel.add(StatusText, c);

        //Display the window.
        frame.pack();
        frame.addWindowListener(new FrameExitListener());
        frame.setVisible(true);

       cameraPrefixText.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent event) {
                if (isDebugMessages)
                    IJ.log("Camera prefix changed");
                disconnectCameraPVs();
                connectCameraPVs();
            }
        });

        transformPrefixText.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent event) {
                if (isDebugMessages)
                    IJ.log("Transform prefix changed");
                disconnectTransformPVs();
                connectTransformPVs();
            }
        });

        roiPrefixText.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent event) {
                if (isDebugMessages)
                    IJ.log("ROI prefix changed");
                disconnectRoiPVs();
                connectRoiPVs();
            }
        });
        
        tweakAmountText.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent event) {
                tweakAmountPixels = Integer.parseInt(tweakAmountText.getText());
                logMessage("Tweak amount is " + tweakAmountPixels, true, true);
            }
        });

        increaseWidthButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent event) {
                
                int size;
                
                if (setCCD_Readout) {
                    try {
                        size = epicsGetInt(ch_sizeCamX);
                        epicsSetInt(ch_sizeCamX, (size + tweakAmountPixels));
                    } catch (CAException ex) {
                        logMessage("increaseWidthButton got CAException: " + ex.getMessage(), true, true);
                    } catch (TimeoutException ex) {
                        logMessage("increaseWidthButton got TimeoutException: " + ex.getMessage(), true, true);
                    } catch (IllegalStateException ex) {
                        logMessage("increaseWidthButton got IllegalStateException: " + ex.getMessage(), true, true);
                    }
                }
                else {
                    try {
                        size = epicsGetInt(ch_sizeRoiX);
                        epicsSetInt(ch_sizeRoiX, size + tweakAmountPixels);           
                    } catch (CAException ex) {
                        logMessage("increaseWidthButton got CAException: " + ex.getMessage(), true, true);
                    } catch (TimeoutException ex) {
                        logMessage("increaseWidthButton got TimeoutException: " + ex.getMessage(), true, true);
                    } catch (IllegalStateException ex) {
                        logMessage("increaseWidthButton got IllegalStateException: " + ex.getMessage(), true, true);
                    }
                }
            }
        });
        
        decreaseWidthButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent event) {
                
                int size;
                
                if (setCCD_Readout) {
                    try {
                        size = epicsGetInt(ch_sizeCamX);
                        epicsSetInt(ch_sizeCamX, size - tweakAmountPixels);           
                    } catch (CAException ex) {
                        logMessage("setROI got CAException: " + ex.getMessage(), true, true);
                    } catch (TimeoutException ex) {
                        logMessage("setROI got TimeoutException: " + ex.getMessage(), true, true);
                    } catch (IllegalStateException ex) {
                        logMessage("setROI got IllegalStateException: " + ex.getMessage(), true, true);
                    }
                }
                else {
                    try {
                        size = epicsGetInt(ch_sizeRoiX);
                        epicsSetInt(ch_sizeRoiX, size - tweakAmountPixels);           
                    } catch (CAException ex) {
                        logMessage("setROI got CAException: " + ex.getMessage(), true, true);
                    } catch (TimeoutException ex) {
                        logMessage("setROI got TimeoutException: " + ex.getMessage(), true, true);
                    } catch (IllegalStateException ex) {
                        logMessage("setROI got IllegalStateException: " + ex.getMessage(), true, true);
                    }
                }                
            }
        });
        
        increaseHeightButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent event) {
                
                int size;
                
                if (setCCD_Readout) {
                    try {
                        size = epicsGetInt(ch_sizeCamY);
                        epicsSetInt(ch_sizeCamY, size + tweakAmountPixels);           
                    } catch (CAException ex) {
                        logMessage("setROI got CAException: " + ex.getMessage(), true, true);
                    } catch (TimeoutException ex) {
                        logMessage("setROI got TimeoutException: " + ex.getMessage(), true, true);
                    } catch (IllegalStateException ex) {
                        logMessage("setROI got IllegalStateException: " + ex.getMessage(), true, true);
                    }
                }
                else {
                    try {
                        size = epicsGetInt(ch_sizeRoiY);
                        epicsSetInt(ch_sizeRoiY, size + tweakAmountPixels);           
                    } catch (CAException ex) {
                        logMessage("setROI got CAException: " + ex.getMessage(), true, true);
                    } catch (TimeoutException ex) {
                        logMessage("setROI got TimeoutException: " + ex.getMessage(), true, true);
                    } catch (IllegalStateException ex) {
                        logMessage("setROI got IllegalStateException: " + ex.getMessage(), true, true);
                    }
                }
                
            }
        });
        
        decreaseHeightButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent event) {
                
                int size;
                
                if (setCCD_Readout) {
                    try {
                        size = epicsGetInt(ch_sizeCamY);
                        epicsSetInt(ch_sizeCamY, size - tweakAmountPixels);           
                    } catch (CAException ex) {
                        logMessage("setROI got CAException: " + ex.getMessage(), true, true);
                    } catch (TimeoutException ex) {
                        logMessage("setROI got TimeoutException: " + ex.getMessage(), true, true);
                    } catch (IllegalStateException ex) {
                        logMessage("setROI got IllegalStateException: " + ex.getMessage(), true, true);
                    }
                }
                else {
                    try {
                        size = epicsGetInt(ch_sizeRoiY);
                        epicsSetInt(ch_sizeRoiY, size - tweakAmountPixels);           
                    } catch (CAException ex) {
                        logMessage("setROI got CAException: " + ex.getMessage(), true, true);
                    } catch (TimeoutException ex) {
                        logMessage("setROI got TimeoutException: " + ex.getMessage(), true, true);
                    } catch (IllegalStateException ex) {
                        logMessage("setROI got IllegalStateException: " + ex.getMessage(), true, true);
                    }
                }
            }
        });
        
        increaseXButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent event) {
                
                int origin;
               
                if (setCCD_Readout) {
                    try {
                        origin = epicsGetInt(ch_minCamX);
                        epicsSetInt(ch_minCamX, origin + tweakAmountPixels);           
                    } catch (CAException ex) {
                        logMessage("setROI got CAException: " + ex.getMessage(), true, true);
                    } catch (TimeoutException ex) {
                        logMessage("setROI got TimeoutException: " + ex.getMessage(), true, true);
                    } catch (IllegalStateException ex) {
                        logMessage("setROI got IllegalStateException: " + ex.getMessage(), true, true);
                    }
                }
                else {
                    try {
                        origin = epicsGetInt(ch_minRoiX);
                        epicsSetInt(ch_minRoiX, origin + tweakAmountPixels);           
                    } catch (CAException ex) {
                        logMessage("setROI got CAException: " + ex.getMessage(), true, true);
                    } catch (TimeoutException ex) {
                        logMessage("setROI got TimeoutException: " + ex.getMessage(), true, true);
                    } catch (IllegalStateException ex) {
                        logMessage("setROI got IllegalStateException: " + ex.getMessage(), true, true);
                    }
                }
            }
        });
        
        decreaseXButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent event) {
                
                int origin;
                
                if (setCCD_Readout) {
                    try {
                        origin = epicsGetInt(ch_minCamX);
                        epicsSetInt(ch_minCamX, origin - tweakAmountPixels);           
                    } catch (CAException ex) {
                        logMessage("setROI got CAException: " + ex.getMessage(), true, true);
                    } catch (TimeoutException ex) {
                        logMessage("setROI got TimeoutException: " + ex.getMessage(), true, true);
                    } catch (IllegalStateException ex) {
                        logMessage("setROI got IllegalStateException: " + ex.getMessage(), true, true);
                    }
                }
                else {
                    try {
                        origin = epicsGetInt(ch_minRoiX);
                        epicsSetInt(ch_minRoiX, origin - tweakAmountPixels);           
                    } catch (CAException ex) {
                        logMessage("setROI got CAException: " + ex.getMessage(), true, true);
                    } catch (TimeoutException ex) {
                        logMessage("setROI got TimeoutException: " + ex.getMessage(), true, true);
                    } catch (IllegalStateException ex) {
                        logMessage("setROI got IllegalStateException: " + ex.getMessage(), true, true);
                    }
                }
            }
        });
        
        increaseYButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent event) {
                
                int origin;
                
                if (setCCD_Readout) {
                    try {
                        origin = epicsGetInt(ch_minCamY);
                        epicsSetInt(ch_minCamY, origin + tweakAmountPixels);           
                    } catch (CAException ex) {
                        logMessage("setROI got CAException: " + ex.getMessage(), true, true);
                    } catch (TimeoutException ex) {
                        logMessage("setROI got TimeoutException: " + ex.getMessage(), true, true);
                    } catch (IllegalStateException ex) {
                        logMessage("setROI got IllegalStateException: " + ex.getMessage(), true, true);
                    }
                }
                else {
                    try {
                        origin = epicsGetInt(ch_minRoiY);
                        epicsSetInt(ch_minRoiY, origin + tweakAmountPixels);           
                    } catch (CAException ex) {
                        logMessage("setROI got CAException: " + ex.getMessage(), true, true);
                    } catch (TimeoutException ex) {
                        logMessage("setROI got TimeoutException: " + ex.getMessage(), true, true);
                    } catch (IllegalStateException ex) {
                        logMessage("setROI got IllegalStateException: " + ex.getMessage(), true, true);
                    }
                }
            }
        });
        
        decreaseYButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent event) {
                
                int origin;
                
                if (setCCD_Readout) {
                    try {
                        origin = epicsGetInt(ch_minCamY);
                        epicsSetInt(ch_minCamY, origin - tweakAmountPixels);           
                    } catch (CAException ex) {
                        logMessage("setROI got CAException: " + ex.getMessage(), true, true);
                    } catch (TimeoutException ex) {
                        logMessage("setROI got TimeoutException: " + ex.getMessage(), true, true);
                    } catch (IllegalStateException ex) {
                        logMessage("setROI got IllegalStateException: " + ex.getMessage(), true, true);
                    }
                }
                else {
                    try {
                        origin = epicsGetInt(ch_minRoiY);
                        epicsSetInt(ch_minRoiY, origin - tweakAmountPixels);           
                    } catch (CAException ex) {
                        logMessage("setROI got CAException: " + ex.getMessage(), true, true);
                    } catch (TimeoutException ex) {
                        logMessage("setROI got TimeoutException: " + ex.getMessage(), true, true);
                    } catch (IllegalStateException ex) {
                        logMessage("setROI got IllegalStateException: " + ex.getMessage(), true, true);
                    }
                }
            }
        });

        resetRoiButton.addActionListener(new ActionListener() {
            /**
             * This method resets both the camera readout region and the
             * plugin ROI to be the full size of the CCD.
             * 
             * @param event 
             */
            @Override
            public void actionPerformed(ActionEvent event) {
                int sizeX;
                int sizeY;

                try {
                    //Get the maximum size of the CCD
                    sizeX = epicsGetInt(ch_maxSizeCamX);
                    sizeY = epicsGetInt(ch_maxSizeCamY);
                    
                    //Reset the camera.
                    if (isCameraConnected)
                        setROI(ch_minCamX, ch_minCamY, ch_sizeCamX, ch_sizeCamY, 0, 0, sizeX, sizeY);
                    //Reset the ROI plugin.
                    if (isRoiConnected)
                        setROI(ch_minRoiX, ch_minRoiY, ch_sizeRoiX, ch_sizeRoiY, 0, 0, sizeX, sizeY);
                    
                } catch (CAException ex) {
                    IJ.log("CAException: Could not reset image ROI to full: " + ex.getMessage());
                } catch (TimeoutException ex) {
                    IJ.log("TimeoutException: Could not reset image ROI to full: " + ex.getMessage());
                } catch (IllegalStateException ex) {
                    IJ.log("IllegalStateException: Could not reset image ROI to full: " + ex.getMessage());
                }
                
            }
        });

        setCCDReadoutRadioButton.addItemListener(new ItemListener () {
            @Override
            public void itemStateChanged(ItemEvent e) {
                setCCD_Readout = e.getStateChange() == ItemEvent.SELECTED;
            }
        });
        
        setROIRadioButton.addItemListener(new ItemListener () {
            @Override
            public void itemStateChanged(ItemEvent e) {
                setROI = e.getStateChange() == ItemEvent.SELECTED;
                preTransformRadioButton.setEnabled(setROI);
                postTransformRadioButton.setEnabled(setROI);
            }
        });
                
        preTransformRadioButton.addItemListener(new ItemListener () {
            @Override
            public void itemStateChanged(ItemEvent e) {
                isPreTransform = e.getStateChange() == ItemEvent.SELECTED;
            }
        });
        
        postTransformRadioButton.addItemListener(new ItemListener () {
            @Override
            public void itemStateChanged(ItemEvent e) {
                isPostTransform = (e.getStateChange() == ItemEvent.SELECTED);
            }
        });
        
        useTransformCheckBox.addItemListener(new ItemListener () {
            @Override
            public void itemStateChanged(ItemEvent e) {
                useTransformPlugin = (e.getStateChange() == ItemEvent.SELECTED);
            }
        });

        useTransformCheckBox.addItemListener(new ItemListener() {
            @Override
            public void itemStateChanged(ItemEvent e) {
                useTransformPlugin = (e.getStateChange() == ItemEvent.SELECTED);
            }
        }
        );

    }

    public class FrameExitListener extends WindowAdapter {

        @Override
        public void windowClosing(WindowEvent event) {
            isPluginRunning = false;
            isNewImageAvailable = false;
            // We need to wake up the main thread so it shuts down cleanly
            synchronized (this) {
                notify();
            }
        }
    }

    /**
     * This method creates a message based on the time and a supplied string.
     * It will optionally write the message to the ImageJ log and/or the
     * status display in the GUI.
     * 
     * @param message      A string that is the message to be displayed.
     * @param logDisplay   Indicates if the message should be displayed on the GUI.
     * @param logFile      Indicates if the message should be displayed on the ImageJ log.
     */
    public void logMessage(String message, boolean logDisplay, boolean logFile) {
        Date date = new Date();
        SimpleDateFormat simpleDate = new SimpleDateFormat("d/M/y k:m:s.S");
        String completeMessage;

        completeMessage = simpleDate.format(date) + ": " + message;
        if (logDisplay) {
            StatusText.setText(completeMessage);
        }
        if (logFile) {
            IJ.log(completeMessage);
        }
    }

    /**
     * This method reads values of PV prefixes from a file and writes them to
     * string variables.
     */
    public void readProperties() {
        String temp, path;

        try {
            String fileSep = System.getProperty("file.separator");
            path = System.getProperty("user.home") + fileSep + propertyFile;
            if (path == null) {
                throw new Exception("No such file: " + path);
            }
            FileInputStream file = new FileInputStream(path);
            properties.load(file);
            file.close();
            temp = properties.getProperty("cameraPrefix");
            if (temp != null) {
                cameraPrefix = temp;
            }
            temp = properties.getProperty("transformPrefix");
            if (temp != null) {
                transformPrefix = temp;
            }
            temp = properties.getProperty("roiPrefix");
            if (temp != null) {
                roiPrefix = temp;
            }
            IJ.log("Read properties file: " + path + "  imagePrefix= " + imagePrefix);
        } catch (Exception ex) {
            IJ.log("readProperties:exception: " + ex.getMessage());
        }
    }

    /**
     * This method writes the prefixes of PVs to a file for later retrieval.
     */
    public void writeProperties() {
        String path;
        try {
            String fileSep = System.getProperty("file.separator");
            path = System.getProperty("user.home") + fileSep + propertyFile;
            properties.setProperty("cameraPrefix", cameraPrefix);
            properties.setProperty("transformPrefix", transformPrefix);
            properties.setProperty("roiPrefix", roiPrefix);
            FileOutputStream file = new FileOutputStream(path);
            properties.store(file, "EPICS_AD_Viewer Properties");
            file.close();
            IJ.log("Wrote properties file: " + path);
        } catch (IOException ex) {
            IJ.log("writeProperties:exception: " + ex.getMessage());
        }
    }
}
