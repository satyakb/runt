# import the necessary modules
import freenect
import cv2
import numpy as np
import serial
import sys
import select

# configure the serial connections (the parameters differs on the device
# you are connecting to)
ser = serial.Serial(
    port='/dev/tty.usbmodem1412',
    baudrate=9600,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
    timeout=0
)

ser.isOpen()

FST = True
RES = (640, 480)
DIMS = (8, 4)

min_depth = 340
max_depth = 700
delta_depth = max_depth - min_depth

def get_video():
    '''Gets RGB image from kinect'''
    array, _ = freenect.sync_get_video()
    array = cv2.cvtColor(array, cv2.COLOR_RGB2BGR)
    return array


def get_depth():
    '''Gets depth image from kinect'''
    array, _ = freenect.sync_get_depth()
    array = array.astype(np.float32)
    return array


def get_centers(res, dims):
    '''
    Divides res into the dims dimension of cells and
    finds the center of each cell
    '''
    max_x, max_y = res
    dim_x, dim_y = dims
    dx = max_x / dim_x
    dy = max_y / dim_y

    start_x = dx / 2
    start_y = dy / 2

    centers = []
    for j in xrange(start_y, max_y, dy):
        for i in reversed(xrange(start_x, max_x, dx)):
            centers.append((i, j))

    return centers


def dist_points(p1, p2):
    '''Calculate distance between 2 points'''
    return np.sqrt((p1[0] - p2[0])**2 + (p1[1] - p2[1])**2)


def control_servo(centroids, depth):
    heights = [(0, 0) for _ in xrange(DIMS[0] * DIMS[1])]
    dx, dy = RES[0] / DIMS[0], RES[1] / DIMS[1]

    for n, centroid in enumerate(centroids):
        x, y = centroid

        servo = 0
        for i in xrange(1, DIMS[0]):
            if x < i * dx:
                servo = DIMS[0] - i
                break

        for i in xrange(DIMS[1] - 1, 0, -1):
            if y > i * dy:
                servo += DIMS[0] * i
                break

        height = ((depth[y, x] - min_depth) / float(delta_depth)) * 100
        height = min(100, height)
        heights[servo] = (height, n)

    start = chr(ord('a') - 2)
    ser.write(start)
    for (height, n) in heights:
        ser.write(chr(int(height) + ord('a')))
        ser.write(chr(int(n) + ord('a')))


def raise_servo(centroids, centers, max_dist, depth):
    '''Communicates height of each servo via serial to mbed'''

    heights = []
    total_dists = []
    for centroid in centroids:
        dists = []
        x,y = centroid
        height = ((depth[y, x] - min_depth) / float(delta_depth))
        height = min(height, 1.0)
        for center in centers:
            dist = int((1 - (dist_points(center, centroid) / max_dist) * height * height * height) * 100)
            dists.append(dist)
        total_dists.append(dists)

    total_dists = np.array(total_dists)
    sums = np.argmax(total_dists, axis=0)

    start = chr(ord('a') - 2)
    ser.write(start)
    for i, s in enumerate(sums):
        # Servo height
        dist = total_dists[s, i]

        chr_dist = chr(int(dist) + ord('a'))
        chr_s = chr(int(s) + ord('a'))
        ser.write(chr_dist)
        ser.write(chr_s)


def get_centroids():
    global FST
    # get a frame from RGB camera
    # frame = get_video()

    # get a frame from depth sensor
    depth = get_depth()

    if FST:
        print 'GO!'
        FST = False

    # Threshold the depth for a binary image. Thresholded at 600 arbitary
    # units
    _, depthThresh = cv2.threshold(
        depth, max_depth, 255, cv2.THRESH_BINARY_INV)

    # depthThresh = cv2.GaussianBlur(depthThresh,(5,5),0)

    # Find and draw contours
    contours, hierarchy = cv2.findContours(depthThresh.astype(
        np.uint8), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    cnts = []
    for contour in contours:
        area = cv2.contourArea(contour)
        if area > 500:
            cnts.append((contour, area))

    # If no contours, don't do anything
    if not cnts:
        return None, None

    cnts = sorted(cnts, key=lambda t: t[1])
    cnts = [cnt[0] for cnt in cnts]

    # Blank image for drawing contours, convexhulls, centroids
    drawing = np.zeros((depth.shape[0], depth.shape[1], 3), np.uint8)

    centroids = []

    # Calculate convex hull, moment, and centroid
    for cnt in cnts:
        hull = cv2.convexHull(cnt, returnPoints=True)
        M = cv2.moments(cnt)
        centroid = int(M['m10'] / M['m00']), int(M['m01'] / M['m00'])
        centroids.append(centroid)

        # cv2.drawContours(drawing, hull, -1, (0, 255, 0), 3)
        cv2.circle(drawing, centroid, 10, (0, 0, 255), -1)

    cv2.drawContours(drawing, cnts, -1, (255, 0, 0), 3)

    # display RGB and depth images
    # cv2.imshow('RGB image',frame)
    # cv2.imshow('Depth image',depthThresh)

    # Show contours
    cv2.imshow('Contours', cv2.flip(drawing, 1))

    return depth, centroids


def main():
    centers = get_centers(RES, DIMS)
    max_dist = dist_points(centers[0], (0, RES[1]))

    # Modes:
    #   0: raise_servo
    #   1: control_servo
    #   2: rainbow
    #   3: snake
    #   4: something else...
    mode = 0

    print 'Setting up...'

    while 1:
        mbed_input = ser.read(1)
        if mbed_input:
            print mbed_input
            try:
                mode = int(mbed_input)
            except ValueError:
                pass

        if mode > 1:
            continue

        # Get centroids from kinect
        depth, centroids = get_centroids()

        if depth is None:
            continue

        # Send height data to servo
        if mode == 0:
            raise_servo(centroids, centers, max_dist, depth)
        elif mode == 1:
            control_servo(centroids, depth)

        # quit program when 'esc' key is pressed
        k = cv2.waitKey(5) & 0xFF
        if k == 27:
            break

    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
