# Windows AI Agent - Frontend

A modern Electron + React application for the Windows AI Agent interface.

## Features

- Modern React UI with beautiful glassmorphism design
- Electron desktop application for Windows
- Hot reload development environment
- Production build with installer generation
- TypeScript-ready architecture

## Prerequisites

- Node.js (version 16 or higher)
- npm or yarn package manager

## Installation

1. Navigate to the frontend directory:

```bash
cd "d:\Windows AI Agent\frontend"
```

2. Install dependencies:

```bash
npm install
```

## Development

Start the development server with hot reload:

```bash
npm run dev
```

This will:

- Start the Vite development server on http://localhost:5173
- Launch the Electron application automatically
- Enable hot reload for both React and Electron

## Available Scripts

- `npm run dev` - Start development mode with hot reload
- `npm run build` - Build the React application for production
- `npm run build:electron` - Build and package the Electron app
- `npm run preview` - Preview the production build
- `npm run electron` - Run Electron with the built application
- `npm run pack` - Package the app without creating installer
- `npm run dist` - Create distributable installer

## Project Structure

```
frontend/
├── electron/           # Electron main process files
│   ├── main.js        # Main Electron process
│   └── preload.js     # Preload script for security
├── src/               # React application source
│   ├── App.jsx        # Main React component
│   ├── App.css        # Application styles
│   ├── main.jsx       # React entry point
│   └── index.css      # Global styles
├── public/            # Static assets
├── dist/              # Built React application (generated)
├── dist-electron/     # Packaged Electron app (generated)
├── package.json       # Project configuration
├── vite.config.js     # Vite configuration
└── electron-builder.json # Electron Builder configuration
```

## Building for Production

1. Build the React application:

```bash
npm run build
```

2. Package the Electron application:

```bash
npm run build:electron
```

The packaged application will be available in the `dist-electron` directory.

## Architecture

This application uses:

- **Electron**: For creating the desktop application
- **React**: For the user interface
- **Vite**: For fast development and building
- **Electron Builder**: For packaging and distribution
- **Lucide React**: For modern icons

## Security

The application follows Electron security best practices:

- Context isolation enabled
- Node integration disabled in renderer
- Preload script for secure IPC communication

## Contributing

1. Make changes to the React components in `src/`
2. Test in development mode with `npm run dev`
3. Build and test the packaged application
4. Submit your changes

## Troubleshooting

### Development server not starting

- Ensure port 5173 is not in use
- Check that all dependencies are installed

### Electron app not launching

- Verify that the Vite server is running first
- Check the console for any error messages

### Build fails

- Ensure all source files are saved
- Clear node_modules and reinstall if needed
