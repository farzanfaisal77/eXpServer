Clone repo
git clone https://github.com/farzanfaisal77/eXpServer

tester:
Setting frontend
cd expserver-tester/frontend/
npm install
create .env.local in frontend/ with the following
NEXT_PUBLIC_BACKEND_URL=http://localhost:6969
NEXT_PUBLIC_SOCKET_URL=http://localhost:6970
build and run:
npm run build
npm run start
setting up backend
go to backend cd expserver-tester/backend/
setup .env.example with your info. add .env.prod with this same details
npm run docker:build
docker compose -f docker-compose.prod.yaml up -d --build #no need build for subseq runs

when starting
npm run start
docker compose -f docker-compose.prod.yaml up

stop:
docker compose -f docker-compose.prod.yaml down
killall node (or ctr + c)