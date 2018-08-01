pipeline {
    agent {
        docker {
            image "gcr.io/organic-storm-201412/fetch-ledger-develop:latest"
        }
    }
    stages {
        stage('Build & Test') {
            steps {
                sh './develop-image/cmake-make.sh CTEST_OUTPUT_ON_FAILURE=1 all test'
                sh './scripts/run-static-analysis.py build/'
                sh './scripts/apply-style.py -w -a'
            }
        }
    }
}

