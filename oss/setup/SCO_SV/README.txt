Build instructions for UW7/OSR6 combined oss package.

A. Build the UW7 version.

    1. On the UW7 machine, create a directory for the builds (we'll use
       $HOME/oss for our examples).

    2. Copy this included tarball into that directory.

    3. Install the txt2man-1.4.8.tar.gz as root:
       # cd /tmp
       # gunzip -c $HOME/oss/txt2man-1.4.8.tar.gz | tar xvf -
       # cd txt2man-1.4.8
       # make install

    4. Make sure there is a gawk installed in /usr/local/bin.  If the
       GNUgawk package is already installed, you can just create a
       symbolic link to it:
       # ln -s /usr/gnu/bin/gawk /usr/local/bin/gawk

    5. Unwind the oss source into the build directory:
       $ cd $HOME/oss
       $ bunzip2 -c /tmp/oss-4.0-177-061108-src.tar.bz2 | tar xvf -
       This should give you a oss-4.0 directory.

    6. Create a directory for the UW7 specific build and do the build and
       package:
       $ mkdir $HOME/oss/oss-4.0-uw7.BUILD
       $ cd $HOME/oss/oss-4.0-uw7.BUILD
       $ $HOME/oss/oss-4.0/configure
       $ make
       $ make package

B. Build an OSR6 version.

    1. On the OSR6 machine, create a directory for the builds (we'll use
       $HOME/oss for our examples).

    2. Copy this included tarball into that directory.

    3. Unwind the oss source into the build directory:
       $ cd $HOME/oss
       $ bunzip2 -c /tmp/oss-4.0-177-061108-src.tar.bz2 | tar xvf -
       This should give you a oss-4.0 directory.

    6. Create a directory for the OSR6 specific build and do the build and
       package:
       $ mkdir $HOME/oss/oss-4.0-osr6.BUILD
       $ cd $HOME/oss/oss-4.0-osr6.BUILD
       $ $HOME/oss/oss-4.0/configure
       $ make
       $ make package

C. Create the combined package.

    1. Copy the entire oss-4.0-osr6.BUILD directory onto the UW7 machine,
       and put it in the $HOME/oss directory.  You should now have both a
       oss-4.0-uw7.BUILD and a oss-4.0-osr6.BUILD directory in $HOME/oss.

    2. Run the mkoss.sh script to build the product.
       $ cd $HOME/oss
       $ ./mkoss.sh

    3. The pkgadd'able data stream is $HOME/oss/oss-4.0-comb/*.pkg.
