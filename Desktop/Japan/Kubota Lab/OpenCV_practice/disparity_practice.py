import cv2 as cv
import numpy as np
from matplotlib import pyplot as plt

left_img = cv.imread("left.jpg", 0)
right_img = cv.imread("right.jpg", 0)

height = np.size(left_img,0)
width = np.size(left_img,1)
new_height = (int)(height/8)
new_width = (int)(width/8)

left_img = cv.resize(left_img,(new_width,new_height))
right_img = cv.resize(right_img,(new_width,new_height))

#cv.imshow("left",left_img)
#cv.imshow("right",right_img)

stereo = cv.StereoBM_create(numDisparities=16,blockSize=21)
disparity = stereo.compute(left_img,right_img)
#plt.imshow(disparity,"gray")
#plt.show()

"""
#Point Graph
camera_matrix_1 = 
dist_coeffs_1 =
camera_matrix_2 = 
dist_coeffs_2 =
imageSize =
rotation_matrix = 
translation_vector = 

R1, R2, P1, P2, Q, validPixROI1, validPixROI2 = cv.stereoRectify(camera_matrix_1, dist_coeffs_1, camera_matrix_2, dist_coeffs_2, imageSize, rotation_matrix, translation_vector)

focal_length = 
Q2 = np.float32([[1,0,0,0],
    [0,-1,0,0],
    [0,0,focal_length*0.05,0], #Focal length multiplication obtained experimentally. 
    [0,0,0,1]])
point_graph = cv.reprojectImageTo3D(disparity, Q)

print(point_graph)
"""

cv.waitKey(0)
cv.destroyAllWindows()