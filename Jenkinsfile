pipeline {
    agent {
        docker {
            image "gcr.io/organic-storm-201412/fetch-ledger-develop:latest"
        }
    }
    stages {
        stage('Test') {
            steps {
                sh './develop-image/cmake-make.sh CTEST_OUTPUT_ON_FAILURE=1 all test'
                sh 'which clang-tidy'
                sh 'which clang-format'
            }
        }
    }
}

