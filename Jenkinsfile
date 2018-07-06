pipeline {
    agent {
        docker {
            image "gcr.io/organic-storm-201412/fetch-ledger-develop:latest"
        }
    }
    stages {
        stage('Test') {
            steps {
                sh 'ls -al && ./develop-image/cmake-make.sh'
            }
        }
    }
}

