# eXpServer Setup & Execution Guide

A comprehensive, step-by-step guide to cloning, configuring, and running the `eXpServer` project (both frontend and backend) for development and production testing.

---

## 1. Cloning the Repository

Begin by cloning the official repository to your local machine:

```zsh
git clone https://github.com/farzanfaisal77/eXpServer
cd eXpServer

```

---

## 2. Setting Up the Frontend

Navigate to the frontend tester directory, install the required dependencies, configure the environment variables, and start the Next.js production server.

### Navigate & Install Dependencies

```zsh
cd expserver-tester/frontend/
npm install

```

### Environment Configuration

Create an `.env.local` file inside the `expserver-tester/frontend/` directory and populate it with the following backend and websocket connection URLs:

```ini
NEXT_PUBLIC_BACKEND_URL=http://localhost:6969
NEXT_PUBLIC_SOCKET_URL=http://localhost:6970

```

### Build and Run

Build the static assets and run the optimized frontend application:

```zsh
npm run build
npm run start

```

---

## 3. Setting Up the Backend

Configure the environment files and build the production Docker containers for the backend service.

### Navigate & Configuration

```zsh
cd ../backend/  # Or cd expserver-tester/backend/ from root

```

Copy the example environment template to create your production configuration:

```zsh
cp .env.example .env.prod

```

> 💡 *Make sure to open `.env.prod` and customize any database credentials or environment keys as required.*

### Build Docker Images

Build the target Docker containers for production:

```zsh
npm run docker:build
docker compose -f docker-compose.prod.yaml up -d --build

```

*(Note: The `--build` flag is only strictly necessary during your initial run or when Dockerfile dependencies change.)*

---

## 4. Running the Application

Once everything is configured, use the following commands to start the backend and frontend systems.


### Start Frontend

Navigate to the frontend directory (if not already running) and execute:

```zsh
cd expserver-tester/frontend/
npm run start

```
### Start Backend

```zsh
cd expserver-tester/backend/
docker compose -f docker-compose.prod.yaml up

```

---

## 5. Stopping the Application

To clean up and shut down the active services gracefully:

### Stop Backend Services

Navigate to the backend directory and spin down the active Docker compose containers:

```zsh
cd expserver-tester/backend/
docker compose -f docker-compose.prod.yaml down

```

### Stop Frontend Server

If the frontend is running in your active shell terminal, press:

* `Ctrl + C`

Alternatively, to force-stop any running node processes:

```zsh
killall node

```

This concludes the end of expserver-tester setup