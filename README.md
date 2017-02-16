# ZEQ2-Lite "Overhaul" Fork

This fork of ZEQ2-Lite's official SVN mirror was set up to make my on-going works available to the public the time everything is ready for an official merge.

# Plans
This "revision 1917" has for goal a complete overhaul of at least the source and tools -- if not the complete repository --, providing new means to produce better content and a more efficient "course" for learning.

Engine side, researches on integrating better or updated libraries are made to get the most efficient formats and APIs available to product developers. The hierarchy and naming are also being restructured for better clearance and intuitivity. I personally think everything product-related should be moved to the 'Virtual Machines' and 'Shared' folders for keeping the engine 'general purpose', or if not used anymore because of contemporary relevance or QVM's initial purpose -- designed for strictly scripted structures --, probably to some dynamically loaded libraries. Modularity in both project structure and runtime is important for long term ease of development and performances.

Actual plans are that the QVM code contains everything related to ZEQ2-Lite, and if possible, as less systems relating to the engine as possible, such as the particles system for example. The higher level the aimed workplace is, the easier it should get! Reconsideration of naming and cleanups are planned, probably even a restructuration to avoid bloated or near empty files. Non-fatal bugs fixing and new mechanics aren't really part of the plans, as they will be thought once the merge is done. Pushing new engine features and maybe even a plan proposal concerning the build are more important.

The 'Shared' folder in the current hierarchy should probably be a bridge between the engine and the product, basically setting up informational data for this last, such as the name, the protocol, net port etc... Algorithms centered on model clipping, VM interpretation or else would likely need to be moved in the engine code.

Tools are being under heavy reworking. The goal is to permit every domains to profit of programs or scripts to help them in their works. It goes from more recent compilers to optimization tools and encodecs.

... to be completed.
