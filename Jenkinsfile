node {
    checkout scm
    sh 'docker run -i --name "damn_thing" fetch-ledger-develop:latest bash -c "sleep 1000000" && develop-image/scripts/jenkins-ci-build.sh'
}

