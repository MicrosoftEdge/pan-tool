
# Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.opensource.microsoft.com.

When you submit a pull request, a CLA bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., status check, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

# About Pan Tool

This is a C++ tool that provides a way of injecting touch input on the primary monitor. 
It sends pointerdown, pointerupdate and pointerup to simulate touch gestures. Depending 
on the arguments passed to the tool, it can result in either tap, pan or a fling gesture.

Arguments that can be passed to the tool:
  * repeat - number of times to repeat the input sequence. The default value is 1.
  * startdelay - number of seconds to wait before starting any input. The default value is 1 second.
  * segmentdelay - number of seconds to wait between each segment of the input sequence. The default value is 3 second.
  * distance - total number of pixels to move in a vertical direction. The default value is 500 pixels.
  * duration - number of second to take to perform the pan gesture. The default value is 1 second.
  * frequency - frequency/frame rate at which input needs to be injected. The default value is 100 frames per second.
  * accelerate - accelerate injection tool instead of linear movement. The default value is false.
  * onedir - do not reverse the pan direction after completing a input sequence. The default value is false
  
Note that touch input is injected at  100, 100 + distance.

Example usage:
1) pan.exe - Inject touch input using default values. This will inject touch gesture with a velocity of 500 pixels/sec.
2) pan.exe distance 400 duration 0.75 repeat 10 - This command-line argument will inject touch gesture with velocity 
                                                  of 533.33 pixels/sec. This input sequence will be repeated 10 with 
                                                  delay of 3 seconds in between consecutive input.
3) pan.exe accelerate - This command-line argument will inject touch gesture with non-linear velocity.
4) pan.exe frequency 60 - This command-line argument will inject touch inputs with a time interval of 16.66ms instead 
                          of 10ms(default value).

Note: This project requires VS 2019 to build.