pipeline {

  agent {
    docker {
      image "gcr.io/organic-storm-201412/fetch-ledger-develop:edtest2"
    }
  }

  stages {

    stage('End 2 End Tests') {
      steps {
        sh 'pip3 freeze'
        sh'echo ---'
        sh './scripts/ci/install-test-dependencies.sh'
        sh 'pip3 freeze'
        sh './scripts/ci-tool.py -BE Release'
      }
    }

  }
}
