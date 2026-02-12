# Certificate extractor

This is a tool to extract the SSL certificate that is used by the Gameforge Client on event requests aswell as to generate the magic number for the `thin/codes` request using [Frida](https://frida.re/).

# How to use

```bash
cd CertExtractor    # Make sure you are in this project folder
npm install         # Install the dependencies
mkdir build         # Create the build directory
npm run build       # Build the javascript agent
npm run launch      # Start the application
```