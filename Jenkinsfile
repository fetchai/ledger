pipeline {

  agent {
    docker {
      image "gcr.io/organic-storm-201412/fetch-ledger-develop:latest"
    }
  }

  stages {

    stage('License Checks') {
        steps {
            sh './scripts/check-license-header.py'
        }
    }

    stage('Clang Format Checks') {
        steps {
            sh './scripts/apply-style.py -w -a'
        }
    }

    stage('Debug Build') {
      steps {
        sh './scripts/ci-tool.py -B Debug'
      }
    }

    stage('Clang Tidy Checks') {
        steps {
            sh './scripts/run-static-analysis.py build-debug/'
        }
    }

    stage('Release Build') {
      steps {
        sh './scripts/ci-tool.py -B Release'
      }
    }


    stage('Unit Tests') {
      steps {
        sh 'CTEST_OUTPUT_ON_FAILURE=1 ./scripts/ci-tool.py -T Release'
      }
    }

  }

  post {
    always {
      junit 'build-release/TestResults.xml'
    }
  }
}

