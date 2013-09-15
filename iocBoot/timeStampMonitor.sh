caput $1:cam1:ArrayCounter_RBV.TSE $2
caput $1:image1:ArrayCounter_RBV.TSE $2
caput $1:image1:UniqueId_RBV.TSE $2
caput $1:image1:ArrayData.TSE $2
caput $1:image1:EpicsTSSec_RBV.TSE $2
caput $1:image1:EpicsTSNsec_RBV.TSE $2
caput $1:ROI1:ArrayCounter_RBV.TSE $2
caput $1:ROI1:UniqueId_RBV.TSE $2
caput $1:Stats1:ArrayCounter_RBV.TSE $2
caput $1:Stats1:UniqueId_RBV.TSE $2
caput $1:Stats1:MeanValue_RBV.TSE $2
camonitor -#1 $1:cam1:ArrayCounter_RBV \
              $1:image1:ArrayCounter_RBV \
              $1:image1:UniqueId_RBV \
              $1:image1:ArrayData \
              $1:image1:EpicsTSSec_RBV \
              $1:image1:EpicsTSNsec_RBV \
              $1:ROI1:ArrayCounter_RBV \
              $1:ROI1:UniqueId_RBV \
              $1:Stats1:ArrayCounter_RBV \
              $1:Stats1:UniqueId_RBV \
              $1:Stats1:MeanValue_RBV \
