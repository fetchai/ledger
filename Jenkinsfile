pipeline {

  agent {
    docker {
      image "gcr.io/organic-storm-201412/fetch-ledger-develop:latest"
    }
  }

  stages {

    stage('Builds') {
      parallel {

        stage('Debug Build') {
          steps {
            sh './scripts/ci-tool.py -B Debug'
          }
        }

        stage('Release Build') {
          steps {
            sh './scripts/ci-tool.py -B Release'
          }
        }

      }
    }

    stage('Unit Tests') {
      steps {
        sh 'CTEST_OUTPUT_ON_FAILURE=1 ./scripts/ci-tool.py -T Release'
      }
    }

    stage('Static Analysis') {
      parallel {

        stage('Clang Tidy Checks') {
            steps {
                sh './scripts/run-static-analysis.py build-release/'
            }
        }

        stage('Clang Format Checks') {
            steps {
                sh './scripts/apply-style.py -w -a'
            }
        }

      }
    }

  }
}

