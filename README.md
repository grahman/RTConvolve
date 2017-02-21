RTCONVOLVE
==========

##Author
Graham Barab
gbarab@gmail.com

##About
RTConvolve is a zero-latency real-time audio effect plugin written in C++ and built on the JUCE framework. It outputs the convolution an input signal with an arbitrary impulse response provided by the user. The goal of this project was to produce a working implementation of an algorithm that performs the computationally expensive operation of convolution with a long impulse response with the constraints that it run in real-time without latency, and that it use only a single thread. It is able to do this by using a combination of uniform and non-uniform partitioning of the impulse response, and by implementing a time-distributed version of the fast Fourier Transform such as that described by Jeffrey R. Hurchalla in his paper "A Time Distributed FFT for Efficient Low Latency Convolution." The plugin is compatible with mono and stereo inputs, and with mono and stereo impulse responses.

##Usage
Use the Projucer application to set the paths for the Juce library modules, then select "Save Project and Open in IDE".

##OS Support
Currently, the jucer file only contains an exporter for a Mac OSX Xcode project. However, aside from the Juce framework, this project uses standard C++11 features. It should therefore be straightforward to create exporters for other operating systems. 
