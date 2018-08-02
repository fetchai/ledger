pipeline {
    agent {
        docker {
            image "gcr.io/organic-storm-201412/fetch-ledger-develop:latest"
        }
    }
    stages {
        stage('Build') {
            steps {
                sh './develop-image/cmake-make.sh CTEST_OUTPUT_ON_FAILURE=1 all'
            }
        }
        stage('Basic Checks') {
            parallel {
                stage('Static Analysis') {
                    steps {
                        sh './develop-image/cmake-make.sh all'
                    }
                }
                stage('Code Style') {
                    steps {
                        sh './scripts/apply-style.py -w -a'
                    }
                }
            }
        }
        stage('Unit Tests') {
            steps {
                sh './develop-image/cmake-make.sh CTEST_OUTPUT_ON_FAILURE=1 test'
            }
        }
    }
}

